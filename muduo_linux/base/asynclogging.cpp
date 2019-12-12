#include"asynclogging.h"
#include"logfile.h"

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize) :
	ringbuffer_(kLargeBuffer * 32),
	end_index_(ringbuffer_.size() - 1),
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

	uint64_t wseq = (write_.seq_ += size) - 1;
	uint64_t begin = end_index_ & (wseq - size + 1);
	uint64_t end = end_index_ & wseq;

	while (wseq > readdone_.seq_ + end_index_ + 1) {
		pthread_yield();
	}
	if (end < begin) {
		uint64_t temp = end_index_ - begin + 1;
		memcpy(&ringbuffer_[begin], src, temp);
		memcpy(&ringbuffer_[0], src + temp, end + 1);
	}
	else
		memcpy(&ringbuffer_[begin], src, size);

	while (wseq - size != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	uint64_t rseq = read_.seq_;
	if (rseq <= wseq && wseq <= (rseq + size)) {
		klock<kmutex> lock(&mutex_);
		cond_.notify();
	}
}

void asynclogger::threadFunc() {
	uint64_t rseq = (read_.seq_ += kLargeBuffer) - 1;
	uint64_t begin = end_index_ & (rseq - kLargeBuffer + 1);
	uint64_t end = end_index_ & rseq;
	logfile log(basename_.c_str(), rollsize_);

	while (running_) {
		if (rseq > writedone_.seq_) {
			klock<kmutex> lock(&mutex_);
			if (rseq > writedone_.seq_)
				cond_.timedwait(&mutex_, 10);
		}

		uint64_t wdseq = writedone_.seq_;
		if (rseq > wdseq) {
			uint64_t wdidx = end_index_ & wdseq;
			if (wdseq > readdone_.seq_) {
				if (wdidx < begin) {
					log.append(&ringbuffer_[begin], end_index_ - begin + 1);
					log.append(&ringbuffer_[0], wdidx + 1);
				}
				else
					log.append(&ringbuffer_[begin], wdidx - begin + 1);

				readdone_.seq_ = wdseq;
				begin = end_index_ & (wdidx + 1);

				log.flush();
			}

			continue;
		}

		if (end < begin) {
			log.append(&ringbuffer_[begin], end_index_ - begin + 1);
			log.append(&ringbuffer_[0], end + 1);
		}
		else
			log.append(&ringbuffer_[begin], end - begin + 1);

		readdone_.seq_ = rseq;
		rseq = (read_.seq_ += kLargeBuffer) - 1;
		begin = end_index_ & (rseq - kLargeBuffer + 1);
		end = end_index_ & rseq;

		log.flush();
	}
}