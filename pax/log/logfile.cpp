#include<pax/log/logfile.h>
#include<pax/log/fileutil.h>

#include<time.h>
#include<assert.h>

using namespace::pax;

/*
namespace {

enum config {

	kPeriodOfRoll = 60 * 60 * 24//s

};

}
*/

logfile::logfile(const char* basename, off_t rollsize) :
	basename_(basename),
	filename_(basename),
	tie_(".A.log"),
	file_(nullptr),
	rollsize_(rollsize),
	last_roll_(0) {

	rollfile();
}

logfile::~logfile() {
	if (file_ != nullptr)
		delete file_;
}

void logfile::flush() {
	file_->flush();
}

void logfile::append_unlock(const char* data, size_t len) {
	size_t leftbytes, temp;

	assert(data);
	while (len) {
		if (file_->writtenBytes() == rollsize_)
			rollfile();

		leftbytes = rollsize_ - file_->writtenBytes();
		if (len < leftbytes)
			temp = len;
		else
			temp = leftbytes;

		file_->append(data, temp);
		data += temp;
		len -= temp;
	}
}

void logfile::rollfile() {
	filename_ = basename_;

	char timebuf[32] = { 0 };
	tm tm_v = { 0 };
	time_t now = ::time(nullptr);

	localtime_r(&now, &tm_v);
	strftime(timebuf, sizeof timebuf, "%Y%m%d-%H%M%S", &tm_v);

	filename_ += timebuf;

	//避免和上一次的文件同名
	if (now == last_roll_) {
		if (tie_[1] == 'Z')
			tie_[1] = 'a';
		else
			tie_[1] = static_cast<char>(tie_[1] + 1);
	}
	else {
		last_roll_ = now;
		tie_[1] = 'A';
	}
	filename_ += tie_;

	if (file_) {
		file_->~appendfile();
		new(file_)appendfile(filename_.c_str());
	}
	else
		file_ = new appendfile(filename_.c_str());
}