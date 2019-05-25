#pragma once

#include"fileutil.h"
#include<pthread.h>
#include<string>
#include<time.h>

class logfile {
public:

	logfile(const char* basename, off_t rollsize, int count_limit = 1024);
	~logfile();

	void append(const char* data, size_t len);
	void flush();
	bool rollfile();//error

private:
	
	static const int kRollPerSeconds = 60 * 60 * 24;
	void setLogFileName(std::string& str);

	void append_unlock(const char* data, size_t len);

	std::string basename_;
	off_t rollsize_;
	time_t time_;
	int count_limit_;
	int count_;

	pthread_mutex_t lock_;
	appendfile* file_;

};
