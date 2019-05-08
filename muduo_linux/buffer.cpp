#include "buffer.h"
#include<stdio.h>
#include<sys/uio.h>

void buffer::swap(buffer& rhs) {
	buffer_.swap(rhs.buffer_);
	std::swap(read_index_, rhs.read_index_);
	std::swap(write_index_, rhs.write_index_);
}

void buffer::append(const char* data, size_t len) {
	ensureWriteable(len);
	std::copy(data, data + len, beginWrite());
}

void buffer::hasWritten(size_t len) {
	assert(len <= writeableBytes());
	write_index_ += len;
}

void buffer::unwrite(size_t len) {
	assert(len <= readableBytes());
	write_index_ -= len;
}

void buffer::ensureWriteable(size_t len) {
	if (writeableBytes() < len)
		makeSpace(len);
	assert(writeableBytes() >= len);
}

void buffer::makeSpace(size_t len) {
	if (writeableBytes() + prependableBytes() < len + kCheapPrepend)
		buffer_.resize(write_index_ + len);
	else {
		assert(kCheapPrepend < read_index_);
		size_t readable_ = readableBytes();
		std::copy(begin() + read_index_, begin() + write_index_, begin() + kCheapPrepend);
		read_index_ = kCheapPrepend;
		write_index_ = read_index_ + readable_;
		assert(readable_ = readableBytes());
	}
}

size_t buffer::readFd(int fd) {
	char extrabuf[65536];
	iovec vec[2];
	const size_t writeable_ = writeableBytes();
	vec[0].iov_base = begin() + write_index_;
	vec[0].iov_len = writeable_;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	
	const int iovcnt = (writeable_ < sizeof extrabuf) ? 2 : 1;
	const size_t n = readv(fd, vec, iovcnt);
	if (n < 0)
		perror("数据接收错误");
	else if (n <= writeable_)
		write_index_ += n;
	else {
		write_index_ = buffer_.size();
		append(extrabuf, n - writeable_);
	}

	return n;
}