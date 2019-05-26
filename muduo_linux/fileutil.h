#pragma once

#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>

class appendfile {
public:
	explicit appendfile(const char* filename);
	~appendfile();

	void append(const char* data, size_t len);
	void flush();
	off_t writtenBytes() { return written_bytes_; }

private:

	FILE* fp_;
	char buffer_[64 * 1024];
	off_t written_bytes_;
};

