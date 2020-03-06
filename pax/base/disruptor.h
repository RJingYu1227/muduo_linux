#pragma once

#include<pax/base/sharedatomic.h>
#include<pax/base/thread.h>

#include<vector>

namespace pax {

//在c++11标准里，class的*运算符的优先级高于++运算符
//在往后的标准里，只高于前置++运算符，而低于后置++运算符
template<typename T>
class disruptor :uncopyable {
public:

	explicit disruptor(size_t capacity);

	void put(const T& val);
	void put(T&& val);
	void take(T& dst);

private:

	sharedatomic<uint64_t> readdone_;//已读末尾下标
	sharedatomic<uint64_t> read_;//可读起始下标
	sharedatomic<uint64_t> writedone_;//已写末尾下标
	sharedatomic<uint64_t> write_;//可写起始下标

	size_t capacity_;
	std::vector<T> ringbuffer_;

	mutex mutex_;
	cond cond_;

};

template<typename T>
disruptor<T>::disruptor(size_t capacity) :
	capacity_(capacity),
	ringbuffer_(capacity) {

	*readdone_ = 0;
	*read_ = 1;
	*writedone_ = 0;
	*write_ = 1;
}

template<typename T>
void disruptor<T>::put(const T& val) {
	uint64_t wseq = *write_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > *readdone_ + capacity_) {
		lock<mutex> lock(&mutex_);

		while (wseq > *readdone_ + capacity_) {
			cond_.wait(&mutex_);
		}
	}
	ringbuffer_[widx] = val;

	while (wseq - 1 != *writedone_) {
		pthread_yield();
	}
	*writedone_ = wseq;

	if (*readdone_ < wseq && wseq < *read_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

template<typename T>
void disruptor<T>::put(T&& val) {
	uint64_t wseq = *write_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > *readdone_ + capacity_) {
		lock<mutex> lock(&mutex_);

		while (wseq > *readdone_ + capacity_) {
			cond_.wait(&mutex_);
		}
	}
	ringbuffer_[widx] = std::move(val);

	while (wseq - 1 != *writedone_) {
		pthread_yield();
	}
	*writedone_ = wseq;

	if (*readdone_ < wseq && wseq < *read_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

template<typename T>
void disruptor<T>::take(T& dst) {
	uint64_t rseq = *read_++;
	uint64_t ridx = (rseq - 1) % capacity_;

	if (rseq > *writedone_) {
		lock<mutex> lock(&mutex_);

		while (rseq > *writedone_) {
			cond_.wait(&mutex_);
		}
	}
	dst = ringbuffer_[ridx];

	while (rseq - 1 != *readdone_) {
		pthread_yield();
	}
	*readdone_ = rseq;

	if (*writedone_ < rseq && rseq < *write_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

}//namespace pax
