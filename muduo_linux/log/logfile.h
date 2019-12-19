#pragma once

#include<string>

class appendfile;

class logfile {
public:

	logfile(const char* basename, off_t rollsize, int count_limit = 1024);
	~logfile();

	void append(const char* data, size_t len);
	void flush();

private:
	
	void rollfile();
	void setLogFileName(std::string& str);
	void append_unlock(const char* data, size_t len);

	std::string basename_;
	appendfile* file_;
	off_t rollsize_;

	int count_;
	int count_limit_;
	long int last_roll_;

	static const int kPeriod = 60 * 60;

};
