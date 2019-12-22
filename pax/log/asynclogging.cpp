#include<pax/log/asynclogging.h>
#include<pax/log/logfile.h>

using namespace::pax;

namespace {

enum config {

	kInterval = 3,//ms
	kRingBufferSize = kLargeBuffer * 32,//byte
	kEndIndex = kRingBufferSize - 1//byte

};

}

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize) :
	ringbuffer_(kRingBufferSize),
	basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogger::threadFunc, this)),
	running_(0) {

	*readdone_ = 0;
	*read_ = 1;
	*writedone_ = 0;
	*write_ = 1;
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
	uint64_t wseq = (*write_ += size) - 1;
	uint64_t begin = kEndIndex & (wseq - size + 1);
	uint64_t end = kEndIndex & wseq;

	while (wseq > *readdone_ + kRingBufferSize) {
		//可写缓冲区大小不足
		cond_.notify();
		pthread_yield();
	}

	//将数据拷贝到环形缓冲区
	if (end < begin) {
		uint64_t temp = kRingBufferSize - begin;
		memcpy(&ringbuffer_[begin], src, temp);
		memcpy(&ringbuffer_[0], src + temp, end + 1);
	}
	else
		memcpy(&ringbuffer_[begin], src, size);
	
	while (wseq - size != *writedone_) {
		//等待其它生产者
		pthread_yield();
	}
	*writedone_ = wseq;
}

void asynclogger::threadFunc() {
	logfile log(basename_.c_str(), rollsize_);
	uint64_t rseq, begin, end;

	auto updateseq = [&, this](uint64_t rdseq) {
		//更新消费者信息
		*readdone_ = rdseq;
		*read_ = rdseq + kLargeBuffer + 1;

		begin = kEndIndex & (rdseq + 1);
		rseq = rdseq + kLargeBuffer;
		end = kEndIndex & rseq;
	};

	updateseq(0);
	for (;;) {
		if (rseq > *writedone_) {
			//等待被唤醒 或者 超时 再消费
			lock<mutex> lock(&mutex_);
			cond_.timedwait(&mutex_, kInterval);
		}

		uint64_t wdseq = *writedone_;
		if (*readdone_ == wdseq) {
			//超时并且缓冲区为空，判断是否需要退出
			if (running_ == false)
				break;
			else
				continue;
		}
		else {
			end = kEndIndex & wdseq;
			rseq = wdseq;
		}

		if (end < begin) {
			log.append(&ringbuffer_[begin], kRingBufferSize - begin);
			log.append(&ringbuffer_[0], end + 1);
		}
		else
			log.append(&ringbuffer_[begin], end - begin + 1);

		updateseq(rseq);
		log.flush();
	}
}