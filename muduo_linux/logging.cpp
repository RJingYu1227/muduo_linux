#include "logging.h"
#include<time.h>
#include<unistd.h>

pthread_once_t logger::async_once_ = PTHREAD_ONCE_INIT;
asynclogging* logger::async_ = nullptr;
std::string logger::log_filename_ = "./RJingYu_LOG.";

void logger::asyncInit() {
	pthread_t temp;
	pthread_create(&temp, NULL, asyncFunc, NULL);
	while (async_ == nullptr);
}

void* logger::asyncFunc(void* a) {
	async_ = new asynclogging(log_filename_.c_str(), logstream::kLargeBuffer * 2);
	async_->start();

	return (void*)0;
}

void logger::output(const char* data, size_t len) {
	pthread_once(&async_once_, asyncInit);
	async_->append(data, len);
}

logger::impl::impl(const char* basename, int line)
	:stream_(),
	basename_(basename),
	line_(line) {

	formatTime();
}

void logger::impl::formatTime() {
	time_t time_ = time(NULL);
	stream_ << ctime(&time_);
}

logger::logger(const char* filename, int line)
	:impl_(filename, line) {

}

logger::~logger() {
	impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
	const logbuffer<logstream::kSmallBuffer>& temp = impl_.stream_.getBuffer();
	output(temp.getData(), temp.length());
}