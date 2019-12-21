#pragma once

#include<string>

namespace pax {

class appendfile;

class logfile {
public:

	logfile(const char* basename, off_t rollsize);
	~logfile();

	void append(const char* data, size_t len) {
		append_unlock(data, len);
	}

	void flush();

private:

	void rollfile();
	void append_unlock(const char* data, size_t len);

	std::string basename_;
	std::string filename_;
	std::string tie_;

	appendfile* file_;
	off_t rollsize_;
	time_t last_roll_;

	//size_t count_;
	//size_t count_limit_;
};

}//namespace pax