#pragma once

#include"logstream.h"

#include<errno.h>

class asynclogging;

class logger {
public:

	logger(const char* filename, int line);
	~logger();

	logstream& stream() { return impl_.stream_; }

	static void setLogFilename(const char* filename) { log_filename_ = filename; }
	static std::string getLogFilenname() { return log_filename_; }
	static bool createAsyncLogger();
	static bool deleteAsyncLogger();//并非线程安全的

private:
	typedef void(*outputFunc)(const char*, size_t, time_t);

	static outputFunc output;
	static void defaultOutput(const char* data, size_t len, time_t time);
	static void asyncOutput(const char* data, size_t len, time_t time);

	static asynclogging* async_;
	static std::string log_filename_;

	struct impl {

		impl(const char* basename, int line);

		logstream stream_;
		std::string basename_;
		int line_;
		time_t time_;
	};

	impl impl_;

};

#define LOG logger(__FILE__, __LINE__).stream()