#pragma once

#include<pax/log/logstream.h>

#include<errno.h>

namespace pax {

class logger :public logstream {
public:
	typedef void(*functor)(const s_logbuffer&);

	logger() {

	}

	~logger() {
		output(getBuffer());
	}

	static const std::string& getFilenname() { return log_filename_; }
	static void setFilename(const char* filename) { log_filename_ = filename; }
	static void setOutput(functor func) { output = func; }

	static void createAsyncLogger();

	static logstream& time(logstream& stream);
	static logstream& flush(logstream& stream);

private:

	static std::string log_filename_;

	static functor output;
	static void defaultOutput(const s_logbuffer&);

};

#define LOGBASE logger() << '[' << logger::time << ' ' << __FILE__ << ' ' << __LINE__ << "]\n" 

#define LOG LOGBASE

}//namespace pax