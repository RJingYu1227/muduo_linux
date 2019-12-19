#pragma once

#include<pax/log/logstream.h>

#include<errno.h>

#include<functional>

namespace pax {

class asynclogger;

class logger {
public:
	typedef std::function<void(const s_logbuffer&)> functor;

	logger(const char* filename, int line);
	~logger();

	logstream& stream() { return impl_.stream_; }

	static std::string getFilenname() { return log_filename_; }
	static void setFilename(const char* filename) { log_filename_ = filename; }

	static void setOutput(const functor& func) { output = func; }
	static void setOutput(functor&& func) { output = std::move(func); }

	static void createAsyncLogger();

private:

	static std::string log_filename_;

	static functor output;
	static void defaultOutput(const s_logbuffer&);

	struct impl {

		impl(const char* basename, int line);

		logstream stream_;
		std::string basename_;
		int line_;

	};

	impl impl_;

};

#define LOG logger(__FILE__, __LINE__).stream()

}//namespace pax