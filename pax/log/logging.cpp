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

logstream& pax::logger::time(logstream& stream) {
	char buff[32] = { 0 };

	tm tm_time;
	time_t seconds = ::time(nullptr);
	localtime_r(&seconds, &tm_time);

	strftime(buff, sizeof buff, "%Y %m %d %H:%M:%S", &tm_time);

	return stream << buff;
}

logstream& pax::logger::flush(logstream& stream) {
	output(stream.getBuffer());
	stream.resetBuffer();

	return stream;
}