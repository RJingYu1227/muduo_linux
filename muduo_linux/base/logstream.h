#pragma once

#include"ktimer.h"

#include<string>
#include<string.h>

template<int SIZE>
class logbuffer {
public:

	logbuffer() :current_(data_) {}
	~logbuffer(){}

	void append(const char* data, size_t len) {
		if (leftBytes() > len) {
			memcpy(current_, data, len);
			current_ += len;
		}
	}

	const char* getData()const { return data_; }
	size_t length()const { return static_cast<size_t>(current_ - data_); }

	char* current() { return current_; }
	size_t leftBytes() { return static_cast<size_t>(endPtr() - current_); }
	void add(size_t len) { current_ += len; }

	void reset() { current_ = data_; }
	void bzero() { memset(data_, 0, sizeof data_); }

	void setTime(time_t time) { time_ = time; }
	time_t getTime()const { return time_; }

private:

	const char* endPtr() { return data_ + sizeof data_; }

	char data_[SIZE];
	char* current_;
	time_t time_;

};

enum SIZE {
	kSmallBuffer = 2048,
	kLargeBuffer = 2048 * 1024
};

typedef logbuffer<kSmallBuffer> s_logbuffer;
typedef logbuffer<kLargeBuffer> l_logbuffer;

class logstream {
public:
	typedef logstream self;

	self& operator<<(bool v) {
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}
	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);
	self& operator<<(const void*);
	self& operator<<(float v) {
		*this << static_cast<double>(v);
		return *this;
	}
	self& operator<<(double);
	self& operator<<(char v) {
		buffer_.append(&v, 1);
		return *this;
	}
	self& operator<<(const char* str) {
		if (str)
			buffer_.append(str, strlen(str));
		else
			buffer_.append("(null)", 6);
		return *this;
	}
	self& operator<<(const std::string& v) {
		buffer_.append(v.c_str(), v.size());
		return *this;
	}

	void appendTime(time_t time) {
		buffer_.setTime(time);
		*this << ktimer::timeToString(time) << '\n';
	}
	void append(const char* data, size_t len) { buffer_.append(data, len); }
	const s_logbuffer& getBuffer() { return buffer_; }
	void resetBuffer() { buffer_.reset(); }

private:

	void staticCheck();

	template<typename T>
	void formatInteger(T);

	s_logbuffer buffer_;

};