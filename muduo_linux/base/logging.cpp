#include"logging.h"
#include"asynclogging.h"
#include"timer.h"

#include<unistd.h>

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

bool logger::createAsyncLogger() {
	if (output == asyncOutput)
		return 0;

	async_ = new asynclogging(log_filename_.c_str(), logstream::kLargeBuffer * 2);
	async_->start();
	output = asyncOutput;

	return 1;
}

bool logger::deleteAsyncLogger() {
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
	
	stream_ << timer::timeToString(timer::getUnixTime()) << '\n';
}

logger::logger(const char* filename, int line)
	:impl_(filename, line) {

}

logger::~logger() {
	impl_.stream_ << " -- " << impl_.basename_ << ' ' << impl_.line_ << '\n';
	const logbuffer<logstream::kSmallBuffer>& temp = impl_.stream_.getBuffer();
	output(temp.getData(), temp.length());
}