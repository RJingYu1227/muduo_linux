#include "buffer.h"
#include<stdio.h>
#include<sys/uio.h>

void buffer::swap(buffer& rhs) {
	buffer_.swap(rhs.buffer_);
	std::swap(begin_index_, rhs.begin_index_);
	std::swap(end_index_, rhs.end_index_);
}

void buffer::append(const char* data, size_t len) {
	ensureWriteable(len);
	std::copy(data, data + len, beginWrite());
}

void buffer::prepend(const void* data, size_t len) {
	assert(len <= begin_index_);
	begin_index_ -= len;
	const char* d = static_cast<const char*>(data);
	std::copy(d, d + len, begin() + begin_index_);
}

void buffer::hasWritten(size_t len) {
	assert(len <= writeableBytes());
	end_index_ += len;
}

void buffer::unwrite(size_t len) {
	assert(len <= readableBytes());
	end_index_ -= len;
}

void buffer::ensureWriteable(size_t len) {
	if (writeableBytes() < len)
		makeSpace(len);
	assert(writeableBytes() >= len);
}

void buffer::makeSpace(size_t len) {
	if (writeableBytes() + prependableBytes() < len + kCheapPrepend)
		buffer_.resize(end_index_ + len);
	else {
		assert(kCheapPrepend < begin_index_);
		size_t readable_ = readableBytes();
		std::copy(begin() + begin_index_, begin() + end_index_, begin() + kCheapPrepend);
		begin_index_ = kCheapPrepend;
		end_index_ = begin_index_ + readable_;
		assert(readable_ = readableBytes());
	}
}

void buffer::retrieve(size_t len) {
	assert(len <= readableBytes());
	if (len < readableBytes())
		begin_index_ += len;
	else
		retrieveAll();
}

void buffer::retrieveAll() {
	begin_index_ = kCheapPrepend;
	end_index_ = kCheapPrepend;
}

size_t buffer::readFd(int fd) {
	char extrabuf[65536];
	iovec vec[2];
	const size_t writeable_ = writeableBytes();
	vec[0].iov_base = begin() + end_index_;
	vec[0].iov_len = writeable_;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	
	const int iovcnt = (writeable_ < sizeof extrabuf) ? 2 : 1;
	const size_t n = readv(fd, vec, iovcnt);
	if (n < 0)
		perror("数据接收错误");
	else if (n <= writeable_)
		end_index_ += n;
	else {
		end_index_ = buffer_.size();
		append(extrabuf, n - writeable_);
	}

	return n;
}