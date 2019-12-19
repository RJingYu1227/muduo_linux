#pragma once

#include<pax/base/thread.h>

#include<atomic>
#include<vector>

namespace pax {

struct sequence {
	enum cache {
		line_size = 64,
		padding_size = line_size - sizeof(std::atomic_uint64_t)
	};

	char front_[padding_size];
	std::atomic_uint64_t seq_;
	char back_[padding_size];

};

template<typename T>
class disruptor :uncopyable {
public:

	explicit disruptor(size_t capacity);

	void put(const T& val);
	void put(T&& val);
	void take(T& dst);

private:

	sequence readdone_;//已读末尾下标
	sequence read_;//可读起始下标
	sequence writedone_;//已写末尾下标
	sequence write_;//可写起始下标

	size_t capacity_;
	std::vector<T> ringbuffer_;

	mutex mutex_;
	cond cond_;

};

template<typename T>
disruptor<T>::disruptor(size_t capacity) :
	capacity_(capacity),
	ringbuffer_(capacity) {

	readdone_.seq_ = 0;
	read_.seq_ = 1;
	writedone_.seq_ = 0;
	write_.seq_ = 1;
}

template<typename T>
void disruptor<T>::put(const T& val) {
	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		lock<mutex> lock(&mutex_);

		while (wseq > readdone_.seq_ + capacity_) {
			cond_.wait(&mutex_);
		}
	}
	ringbuffer_[widx] = val;

	while (wseq - 1 != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	if (readdone_.seq_ < wseq && wseq < read_.seq_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

template<typename T>
void disruptor<T>::put(T&& val) {
	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		lock<mutex> lock(&mutex_);

		while (wseq > readdone_.seq_ + capacity_) {
			cond_.wait(&mutex_);
		}
	}
	ringbuffer_[widx] = std::move(val);

	while (wseq - 1 != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	if (readdone_.seq_ < wseq && wseq < read_.seq_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

template<typename T>
void disruptor<T>::take(T& dst) {
	uint64_t rseq = read_.seq_++;
	uint64_t ridx = (rseq - 1) % capacity_;

	if (rseq > writedone_.seq_) {
		lock<mutex> lock(&mutex_);

		while (rseq > writedone_.seq_) {
			cond_.wait(&mutex_);
		}
	}
	dst = ringbuffer_[ridx];

	while (rseq - 1 != readdone_.seq_) {
		pthread_yield();
	}
	readdone_.seq_ = rseq;

	if (writedone_.seq_ < rseq && rseq < write_.seq_) {
		lock<mutex> lock(&mutex_);

		cond_.notifyAll();
	}
}

}//namespace pax