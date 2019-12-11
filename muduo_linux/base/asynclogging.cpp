#include"asynclogging.h"
#include"logfile.h"

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize) :
	ringbuffer_(1024 * 1024),
	end_index_(ringbuffer_.size() - 1),
	thread_logfile_(kthreadlocal<logfile>::freeFunc),
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
	logfile* log = thread_logfile_.get();
	if (log == nullptr) {
		log = new logfile((basename_ + std::to_string(pthread_self()) + '.').c_str(), rollsize_);
		thread_logfile_.set(log);
	}

	uint64_t wseq = write_.seq_++;
	uint64_t widx = wseq & end_index_;

	while (wseq > readdone_.seq_ + end_index_ + 1) {
		pthread_yield();
	}
	ringbuffer_[widx].first = log;
	ringbuffer_[widx].second.assign(sbuff.getData(), sbuff.length());

	while (wseq - 1 != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	if (wseq < read_.seq_) {
		klock<kmutex> lock(&mutex_);
		cond_.notify();
	}
}

void asynclogger::threadFunc() {
	uint64_t rseq = read_.seq_++;
	uint64_t ridx = rseq & end_index_;
	logfile* log;

	while (running_) {
		if (rseq > writedone_.seq_) {
			klock<kmutex> lock(&mutex_);
			if (rseq > writedone_.seq_)
				cond_.timedwait(&mutex_, 10);
		}

		if (rseq > writedone_.seq_)
			continue;

		log = ringbuffer_[ridx].first;
		log->append(ringbuffer_[ridx].second.c_str(), ringbuffer_[ridx].second.size());

		readdone_.seq_ = rseq;

		log->flush();

		rseq = read_.seq_++;
		ridx = rseq & end_index_;
	}
}