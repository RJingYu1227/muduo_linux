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
		begin_index_(kCheapPrepend),
		end_index_(kCheapPrepend) {

	}
	~buffer() {}

	void swap(buffer& rhs);
	void append(const char* data, size_t len);
	void prepend(const void* data, size_t len);//用于封包

	size_t readableBytes()const { return end_index_ - begin_index_; }
	size_t writeableBytes()const { return buffer_.size() - end_index_; }
	size_t prependableBytes()const { return begin_index_; }
	size_t capacity()const { return buffer_.capacity(); }

	char* beginWrite() { return begin() + end_index_; }
	const char* beginWrite() const { return begin() + end_index_; }
	void hasWritten(size_t len);
	void unwrite(size_t len);
	void ensureWriteable(size_t len);
	void retrieve(size_t len);
	void retrieveAll();

	size_t readFd(int fd);
	const char* peek()const { return begin() + begin_index_; }

private:
	char* begin() { return &*buffer_.begin(); }
	const char* begin() const { return &*buffer_.begin(); }

	void makeSpace(size_t len);

	std::vector<char> buffer_;
	size_t begin_index_;
	size_t end_index_;
};

