#pragma once

#include<vector>
#include<string>
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

	size_t usedBytes()const { return end_index_ - begin_index_; }
	size_t leftBytes()const { return buffer_.size() - end_index_; }
	size_t prependableBytes()const { return begin_index_; }
	size_t capacity()const { return buffer_.capacity(); }

	char* endCharPtr() { return headCharPtr() + end_index_; }
	const char* endCharPtr() const { return headCharPtr() + end_index_; }
	void hasUsed(size_t len);
	void unUsed(size_t len);
	void ensureLeftBytes(size_t len);
	void retrieve(size_t len);
	void retrieveAll();

	size_t readFd(int fd);
	const char* beginCharPtr()const { return headCharPtr() + begin_index_; }
	std::string toString();

private:
	char* headCharPtr() { return &*buffer_.begin(); }
	const char* headCharPtr() const { return &*buffer_.begin(); }

	void makeSpace(size_t len);

	std::vector<char> buffer_;
	size_t begin_index_;
	size_t end_index_;
};

