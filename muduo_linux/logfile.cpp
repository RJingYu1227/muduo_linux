#include "logfile.h"
#include<assert.h>

logfile::logfile(const char* basename, off_t rollsize, int count_limit)
	:basename_(basename),
	rollsize_(rollsize),
	time_(0),
	count_limit_(count_limit),
	count_(0),
	lock_(PTHREAD_MUTEX_INITIALIZER),
	file_(nullptr) {

	//assert(basename_.find('/') == std::string::npos);
	rollfile();
}

logfile::~logfile() {
	if (file_ != nullptr)
		delete file_;
}

void logfile::append(const char* data, size_t len) {
	//pthread_mutex_lock(&lock_);

	append_unlock(data, len);

	//pthread_mutex_unlock(&lock_);
}

void logfile::flush() {
	//pthread_mutex_lock(&lock_);

	file_->flush();

	//pthread_mutex_unlock(&lock_);
}

void logfile::append_unlock(const char* data, size_t len) {
	file_->append(data, len);
	if (file_->writtenBytes() > rollsize_)
		rollfile();
	else {
		++count_;
		if (count_ > count_limit_) {
			count_ = 0;
			rollfile();
		}
	}
}

bool logfile::rollfile() {
	std::string filename; 
	setLogFileName(filename);
	if (file_ == nullptr) {
		file_ = new appendfile(filename.c_str());
		return 1;
	}
	if (file_) {
		file_->~appendfile();
		new(file_)appendfile(filename.c_str());
		return 1;
	}
	return 0;
}

void logfile::setLogFileName(std::string& str) {
	str = basename_;

	time_ = time(NULL);
	str += ctime(&time_);

	str += ".log";
}