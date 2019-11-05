﻿#include"logging.h"
#include"asynclogging.h"
#include"ktimer.h"

#include<unistd.h>

std::string logger::log_filename_ = "./rjingyu_log/RJingYu.";
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

	impl_.stream_ << ktimer::timeToString(ktimer::getUnixTime()) << '\n';
}

logger::~logger() {
	impl_.stream_ << " -- " << impl_.basename_ << ' ' << impl_.line_ << '\n';
	output(impl_.stream_.getBuffer());
}