#pragma once

#include<vector>
#include<string.h>
#include<assert.h>
#include<algorithm>

class buffer
{
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;

	explicit buffer(size_t initial_size = kInitialSize)
		:buffer_(kCheapPrepend + initial_size),
		read_index_(kCheapPrepend),
		write_index_(kCheapPrepend) {

	}
	~buffer() {}

	void swap(buffer& rhs);
	void append(const char* data, size_t len);

	size_t readableBytes()const { return write_index_ - read_index_; }
	size_t writeableBytes()const { return buffer_.size() - write_index_; }
	size_t prependableBytes()const { return read_index_; }
	size_t capacity()const { return buffer_.capacity(); }

	char* beginWrite() { return begin() + write_index_; }
	const char* beginWrite() const { return begin() + write_index_; }
	void hasWritten(size_t len);
	void unwrite(size_t len);
	void ensureWriteable(size_t len);

	size_t readFd(int fd);
	const char* peek()const { return begin() + read_index_; }


private:
	char* begin() { return &*buffer_.begin(); }
	const char* begin() const { return &*buffer_.begin(); }

	void makeSpace(size_t len);

	std::vector<char> buffer_;
	size_t read_index_;
	size_t write_index_;
};

