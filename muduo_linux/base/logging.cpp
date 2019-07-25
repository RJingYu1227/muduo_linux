#include"logging.h"
#include"asynclogging.h"

#include<unistd.h>

logger::outputFunc logger::output = logger::defaultOutput;
asynclogging* logger::async_ = nullptr;
std::string logger::log_filename_ = "./RJingYu.";

void logger::defaultOutput(const logstream::s_logbuffer& temp) {
	fwrite(temp.getData(), 1, temp.length(), stdout);
	fflush(stdout);
}

void logger::asyncOutput(const logstream::s_logbuffer& temp) {
	async_->append(temp);
}

bool logger::createAsyncLogger() {
	if (output == asyncOutput)
		return 0;

	async_ = new asynclogging(log_filename_.c_str(), logstream::kLargeBuffer * 2);
	async_->start();
	output = asyncOutput;

	LOG << "创建asyncLogger";
	return 1;
}

bool logger::deleteAsyncLogger() {
	if (output == defaultOutput)
		return 0;

	output = defaultOutput;
	delete async_;
	async_ = nullptr;

	LOG << "关闭asyncLogger";
	return 1;
}

logger::impl::impl(const char* basename, int line)
	:basename_(basename),
	line_(line) {

	stream_.appendTime(ktimer::getUnixTime());
}

logger::logger(const char* filename, int line)
	:impl_(filename, line) {

}

logger::~logger() {
	impl_.stream_ << " -- " << impl_.basename_ << ' ' << impl_.line_ << '\n';
	output(impl_.stream_.getBuffer());
}