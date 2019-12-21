#include<pax/log/asynclogging.h>
#include<pax/log/logfile.h>

using namespace::pax;

namespace {

enum config {

	kInterval = 10,//ms
	kBufferSize = kLargeBuffer * 32,//byte
	kEndIndex = kBufferSize - 1//byte

};

}

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize) :
	ringbuffer_(kBufferSize),
	basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogger::threadFunc, this)),
	running_(0) {

	readdone_.seq_ = 0;
	read_.seq_ = 1;
	writedone_.seq_ = 0;
	write_.seq_ = 1;
}

asynclogger::~asynclogger() {
	if (running_) {
		running_ = 0;
		thread_.join();
	}
}

void asynclogger::append(const s_logbuffer& sbuff) {
	size_t size = sbuff.length();
	const char* src = sbuff.getData();

	if (size == 0)
		return;

	//更新生产者信息
	uint64_t wseq = (write_.seq_ += size) - 1;
	uint64_t begin = kEndIndex & (wseq - size + 1);
	uint64_t end = kEndIndex & wseq;

	while (wseq > readdone_.seq_ + kBufferSize) {
		//可写缓冲区大小不足
		pthread_yield();
	}

	//将数据拷贝到环形缓冲区
	if (end < begin) {
		uint64_t temp = kBufferSize - begin;
		memcpy(&ringbuffer_[begin], src, temp);
		memcpy(&ringbuffer_[0], src + temp, end + 1);
	}
	else
		memcpy(&ringbuffer_[begin], src, size);

	while (wseq - size != writedone_.seq_) {
		//等待其它生产者
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	uint64_t rseq = read_.seq_;
	if (rseq <= wseq && wseq <= (rseq + size)) {
		//缓冲区写入的数据的大小刚好达到要求
		lock<mutex> lock(&mutex_);
		cond_.notify();
	}
}

void asynclogger::threadFunc() {
	logfile log(basename_.c_str(), rollsize_);
	
	auto logappend = [&, this](uint64_t begin, uint64_t end) {
		//将环形缓冲区的数据添加到log，log会将其写入磁盘
		if (end < begin) {
			log.append(&ringbuffer_[begin], kBufferSize - begin);
			log.append(&ringbuffer_[0], end + 1);
		}
		else
			log.append(&ringbuffer_[begin], end - begin + 1);
	};

	uint64_t rseq, begin, end;

	auto updateseq = [&, this](uint64_t rdseq) {
		//更新消费者信息
		readdone_.seq_ = rdseq;
		read_.seq_ = rdseq + kLargeBuffer + 1;

		begin = kEndIndex & (rdseq + 1);
		rseq = rdseq + kLargeBuffer;
		end = kEndIndex & rseq;
	};

	updateseq(0);
	for (;;) {
		if (rseq > writedone_.seq_) {
			//等待缓冲区写入的数据的大小达到要求 或者 超时 再消费
			lock<mutex> lock(&mutex_);
			if (rseq > writedone_.seq_)
				cond_.timedwait(&mutex_, kInterval);
		}

		uint64_t wdseq = writedone_.seq_;
		if (readdone_.seq_ == wdseq) {
			//超时并且缓冲区为空，判断是否需要退出
			if (running_ == false)
				break;
		}
		else if (wdseq < rseq) {
			//超时并且缓冲区写入一定的数据，但数据的大小未达到要求
			uint64_t wdidx = kEndIndex & wdseq;

			logappend(begin, wdidx);
			updateseq(wdseq);
			log.flush();
		}
		else {
			//缓冲区写入的数据的大小达到要求
			logappend(begin, end);
			updateseq(rseq);
			log.flush();
		}
	}
}