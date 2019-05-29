#include "logging.h"
#include<time.h>
#include<unistd.h>
#include<pthread.h>

logger::outputFunc logger::output = logger::defaultOutput;
asynclogging* logger::async_ = nullptr;
std::string logger::log_filename_ = "./RJingYu_LOG.";

void logger::defaultOutput(const char* data, size_t len) {
	fwrite(data, 1, len, stdout);
	fflush(stdout);
}

void logger::asyncOutput(const char* data, size_t len) {
	async_->append(data, len);
}

void* logger::asyncFunc(void* a) {
	async_ = new asynclogging(log_filename_.c_str(), logstream::kLargeBuffer * 2);
	async_->start();

	return (void*)0;
}

bool logger::createAsyncLogging() {
	if (output == asyncOutput)
		return 0;

	pthread_t tid;
	int ret = pthread_create(&tid, NULL, asyncFunc, NULL);
	if (ret) {
		LOG << "日志线程创建失败";
		return 0;
	}
	while (async_ == nullptr);

	output = asyncOutput;
	LOG << "创建日志线程成功，线程为：" << tid;
	return 1;
}

bool logger::deleteAsyncLogging() {
	if (output == defaultOutput)
		return 0;

	output = defaultOutput;
	delete async_;
	async_ = nullptr;
	return 1;
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
	impl_.stream_ << " -- " << impl_.basename_ << ' ' << impl_.line_ << '\n';
	const logbuffer<logstream::kSmallBuffer>& temp = impl_.stream_.getBuffer();
	output(temp.getData(), temp.length());
}