#include"buffer.h"
#include<stdio.h>
#include<assert.h>
#include<sys/uio.h>

const char buffer::kCRLF[3] = "\r\n";

void buffer::swap(buffer& rhs) {
	buffer_.swap(rhs.buffer_);
	std::swap(begin_index_, rhs.begin_index_);
	std::swap(end_index_, rhs.end_index_);
}

void buffer::append(const char* data, size_t len) {
	ensureLeftBytes(len);
	std::copy(data, data + len, endPtr());
	hasUsed(len);
}

void buffer::prepend(const void* data, size_t len) {
	assert(len <= begin_index_);
	begin_index_ -= len;
	const char* d = static_cast<const char*>(data);
	std::copy(d, d + len, headPtr() + begin_index_);
}

void buffer::hasUsed(size_t len) {
	assert(len <= leftBytes());
	end_index_ += len;
}

void buffer::unUsed(size_t len) {
	assert(len <= usedBytes());
	end_index_ -= len;
}

void buffer::ensureLeftBytes(size_t len) {
	if (leftBytes() < len)
		makeSpace(len);
	assert(leftBytes() >= len);
}

void buffer::makeSpace(size_t len) {
	if (leftBytes() + prependableBytes() < len + kCheapPrepend)
		buffer_.resize(end_index_ + len);
	else {
		assert(kCheapPrepend < begin_index_);
		size_t readable_ = usedBytes();
		std::copy(headPtr() + begin_index_, headPtr() + end_index_, headPtr() + kCheapPrepend);
		begin_index_ = kCheapPrepend;
		end_index_ = begin_index_ + readable_;
		assert(readable_ = usedBytes());
	}
}

void buffer::retrieve(size_t len) {
	assert(len <= usedBytes());
	if (len < usedBytes())
		begin_index_ += len;
	else
		retrieveAll();
}

void buffer::retrieveAll() {
	begin_index_ = kCheapPrepend;
	end_index_ = kCheapPrepend;
}

ssize_t buffer::readFd(int fd) {
	char extrabuf[65536];
	iovec vec[2];
	const size_t writeable_ = leftBytes();
	vec[0].iov_base = headPtr() + end_index_;
	vec[0].iov_len = writeable_;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	
	const int iovcnt = (writeable_ < sizeof extrabuf) ? 2 : 1;
	const ssize_t n = readv(fd, vec, iovcnt);
	if (n < 0)
		;//完善这里
	else if (static_cast<size_t>(n) <= writeable_)
		end_index_ += n;
	else {
		end_index_ = buffer_.size();
		append(extrabuf, n - writeable_);
	}

	return n;
}

std::string buffer::toString() {
	std::string s(beginPtr(), usedBytes());
	return s;
}

//search为左闭右开区间
const char* buffer::findCRLF()const {
	const char* crlf = std::search(beginPtr(), endPtr(), kCRLF, kCRLF + 2);
	return crlf == endPtr() ? NULL : crlf;
}

const char* buffer::findCRLF(const char* start)const {
	assert(start >= beginPtr() && start <= endPtr());
	const char* crlf = std::search(start, endPtr(), kCRLF, kCRLF + 2);
	return crlf == endPtr() ? NULL : crlf;
}