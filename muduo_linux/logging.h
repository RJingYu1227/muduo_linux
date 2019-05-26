#pragma once

#include"logstream.h"
#include"asynclogging.h"
#include<string>

class logstream;
class asynclogging;

class logger {
public:

	logger(const char* filename, int line);
	~logger();

	logstream& stream() { return impl_.stream_; }

	static void setLogFilename(const char* filename) { log_filename_ = filename; }
	static std::string getLogFilenname() { return log_filename_; }
	static bool createAsyncLogging();

private:
	typedef void(*outputFunc)(const char* data, size_t len);

	static outputFunc output;
	static void defaultOutput(const char* data, size_t len);
	static void asyncOutput(const char* data, size_t len);

	static asynclogging* async_;
	static std::string log_filename_;
	static void* asyncFunc(void* a);

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