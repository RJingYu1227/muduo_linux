#pragma once

#include<vector>
#include<string>
#include<algorithm>

class buffer
{
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024;
	const char kCRLF[3] = "\r\n";

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

	char* endPtr() { return headPtr() + end_index_; }
	const char* endPtr() const { return headPtr() + end_index_; }
	void hasUsed(size_t len);
	void unUsed(size_t len);
	void ensureLeftBytes(size_t len);
	void retrieve(size_t len);
	void retrieveAll();

	size_t readFd(int fd);
	const char* beginPtr()const { return headPtr() + begin_index_; }
	std::string toString();

	const char* findCRLF()const;
	const char* findCRLF(const char* start)const;

private:
	char* headPtr() { return &*buffer_.begin(); }
	const char* headPtr() const { return &*buffer_.begin(); }

	void makeSpace(size_t len);

	std::vector<char> buffer_;
	size_t begin_index_;
	size_t end_index_;
};

