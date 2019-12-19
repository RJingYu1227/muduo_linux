#pragma once

#include"base/uncopyable.h"

#include<unistd.h>
#include<stdio.h>

namespace pax {

class appendfile :uncopyable {
public:
	explicit appendfile(const char* filename);
	~appendfile() {
		fclose(fp_);
	}

	void append(const char* data, size_t len);
	void flush() { fflush(fp_); }
	off_t writtenBytes() { return written_bytes_; }

private:

	FILE* fp_;
	char buffer_[64 * 1024];
	off_t written_bytes_;
};

}//namespace pax