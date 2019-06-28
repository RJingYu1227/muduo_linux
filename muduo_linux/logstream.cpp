#include"logstream.h"

#include<algorithm>
#include<stdio.h>

const char logDigits[] = "9876543210123456789";
const char* logZero = logDigits + 9;
const char logDigitsHex[] = "0123456789ABCDEF";

template<typename T>
size_t logConvert(char buf[], T value){
	T i = value;
	char* p = buf;

	do {
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = logZero[lsd];
	} while (i != 0);

	if (value < 0)
		*p++ = '-';
	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

size_t logConvertHex(char buf[], uintptr_t value) {
	uintptr_t i = value;
	char* p = buf;

	do {
		int lsd = static_cast<int>(i % 16);
		i /= 16;
		*p++ = logDigitsHex[lsd];
	} while (i != 0);

	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

//这两个转换函数是复制muduo的

template class logbuffer<logstream::kSmallBuffer>;
template class logbuffer<logstream::kLargeBuffer>;

void logstream::staticCheck() {
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
		"kMaxNumericSize is large enough");
}

template<typename T>
void logstream::formatInteger(T v) {
	if (buffer_.leftBytes() >= kMaxNumericSize) {
		size_t len = logConvert(buffer_.current(), v);
		buffer_.add(len);
	}
}

logstream& logstream::operator<<(short v) {
	*this << static_cast<int>(v);
	return *this;
}

logstream& logstream::operator<<(unsigned short v) {
	*this << static_cast<unsigned int>(v);
	return *this;
}

logstream& logstream::operator<<(int v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(unsigned int v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(long v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(unsigned long v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(long long v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(unsigned long long v) {
	formatInteger(v);
	return *this;
}

logstream& logstream::operator<<(double v) {
	if (buffer_.leftBytes() >= kMaxNumericSize) {
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
		//%g是根据结果自动选择科学记数法还是一般的小数记数法
		buffer_.add(len);
	}
	return *this;
}

logstream& logstream::operator<<(const void* p) {
	uintptr_t v = reinterpret_cast<uintptr_t>(p);

	if (buffer_.leftBytes() >= kMaxNumericSize) {
		char* buf = buffer_.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = logConvertHex(buf + 2, v);
		buffer_.add(len + 2);
	}
	return *this;
}
