#include<pax/log/logfile.h>
#include<pax/log/fileutil.h>

#include<time.h>

using namespace::pax;

logfile::logfile(const char* basename, off_t rollsize, int count_limit)
	:basename_(basename),
	file_(nullptr),
	rollsize_(rollsize),
	count_(0),
	count_limit_(count_limit),
	last_roll_(time(NULL)) {

	rollfile();
}

logfile::~logfile() {
	if (file_ != nullptr)
		delete file_;
}

void logfile::append(const char* data, size_t len) {
	append_unlock(data, len);
}

void logfile::flush() {
	file_->flush();
}

void logfile::append_unlock(const char* data, size_t len) {
	file_->append(data, len);
	if (file_->writtenBytes() > rollsize_)
		rollfile();
	else {
		++count_;
		if (count_ > count_limit_) {
			time_t now = time(NULL);
			if (now - last_roll_ >= kPeriod)
				rollfile();
		}
	}
}

void logfile::rollfile() {
	std::string filename; 
	setLogFileName(filename);

	if (file_ == nullptr)
		file_ = new appendfile(filename.c_str());
	else {
		file_->~appendfile();
		new(file_)appendfile(filename.c_str());
	}
	count_ = 0;
	last_roll_ = time(NULL);
}

void logfile::setLogFileName(std::string& str) {
	str = basename_;

	char timebuf[32] = { 0 };
	time_t time_ = time(NULL);
	tm tm_;
	localtime_r(&time_, &tm_);
	strftime(timebuf, sizeof timebuf, "%Y%m%d-%H%M%S", &tm_);

	str += timebuf;
	str += ".log";
}