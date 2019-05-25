#pragma once

#include"logstream.h"
#include"asynclogging.h"
#include<string>
#include<pthread.h>

class logstream;
class asynclogging;

class logger {
public:

	logger(const char* filename, int line);
	~logger();

	logstream& stream() { return impl_.stream_; }

	static void setLogFilename(const char* filename) { log_filename_ = filename; }
	static std::string getLogFilenname() { return log_filename_; }

private:
	
	static void output(const char* data, size_t len);
	static void asyncInit();
	static void* asyncFunc(void* a);
	static pthread_once_t async_once_;
	static asynclogging* async_;
	static std::string log_filename_;

	struct impl {

		impl(const char* basename, int line);
		void formatTime();

		logstream stream_;
		std::string basename_;
		int line_;

	};

	impl impl_;

};

#define LOG logger(__FILE__, __LINE__).stream()