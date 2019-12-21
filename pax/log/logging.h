#pragma once

#include<pax/log/logstream.h>

#include<errno.h>

namespace pax {

class logger :logstream {
public:
	typedef void(*functor)(const s_logbuffer&);

	logger() {

	}

	~logger() {
		*this << logger::flush;
	}

	template<typename T>
	logger& operator<<(T v) {
		logstream::operator<<(v);

		return *this;
	}

	logger& operator<<(logger&(*pf)(logger&)) {
		return pf(*this);
	}

	static const std::string& getFilenname() { return log_filename_; }
	static void setFilename(const char* filename) { log_filename_ = filename; }
	static void setOutput(functor func) { output = func; }

	static void createAsyncLogger();

	static logger& time(logger& log);
	static logger& flush(logger& log);
	static logger& error(logger& log);

private:

	static std::string log_filename_;

	static functor output;
	static void defaultOutput(const s_logbuffer&);

};

#define LOG_HEAD_BASE '[' << logger::time << ' ' << __FILE__ << ' ' << __LINE__ << "]\n"

#define LOG_HEAD LOG_HEAD_BASE

#define LOG_BASE logger() << LOG_HEAD 

#define LOG LOG_BASE

}//namespace pax