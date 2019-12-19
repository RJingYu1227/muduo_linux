#include<pax/log/logging.h>
#include<pax/log/asynclogging.h>

#include<unistd.h>
#include<time.h>

using namespace::pax;

std::string logger::log_filename_ = "./RJingYu.";
logger::functor logger::output = logger::defaultOutput;

namespace {

	asynclogger* async_ = nullptr;

	void asyncOutput(const s_logbuffer& temp) {
		async_->append(temp);
	}

}

void logger::defaultOutput(const s_logbuffer& temp) {
	fwrite(temp.getData(), 1, temp.length(), stdout);
	fflush(stdout);
}

void logger::createAsyncLogger() {
	async_ = asynclogger::initialize(log_filename_.c_str(), kLargeBuffer * 2);
	async_->start();
	output = asyncOutput;

	LOG << "创建asyncLogger";
}

logger::impl::impl(const char* basename, int line)
	:basename_(basename),
	line_(line) {

}

logger::logger(const char* filename, int line)
	:impl_(filename, line) {

	char timebuf[32] = { 0 };
	tm tm_time;
	time_t seconds = time(nullptr);
	localtime_r(&seconds, &tm_time);
	strftime(timebuf, sizeof timebuf, "%Y %m %d %H:%M:%S", &tm_time);

	impl_.stream_ << timebuf << '\n';
}

logger::~logger() {
	impl_.stream_ << " -- " << impl_.basename_ << ' ' << impl_.line_ << '\n';
	output(impl_.stream_.getBuffer());
}