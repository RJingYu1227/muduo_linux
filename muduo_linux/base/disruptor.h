#pragma once

#include"kthread.h"

#include<atomic>
#include<vector>

struct sequence {
	enum cache {
		line_size = 64,
		padding_size = 64 - sizeof(std::atomic_uint64_t)
	};

	char front_[padding_size];
	std::atomic_uint64_t seq_;
	char back_[padding_size];

};

template<typename T>
class disruptor :uncopyable {
public:

	disruptor(size_t capacity);
	
	void put(const T& val);
	void put(T&& val);
	void take(T& dst);

private:

	typedef std::pair<kcond, kmutex> channel;
	typedef std::pair<channel*, T> impl;

	sequence readdone_;//已读末尾下标
	sequence read_;//可读起始下标
	sequence writedone_;//已写末尾下标
	sequence write_;//可写起始下标

	std::vector<impl> ringbuffer_;
	size_t capacity_;

	kthreadlocal<channel> channel_;
};

template<typename T>
disruptor<T>::disruptor(size_t capacity) :
	ringbuffer_(capacity),
	capacity_(ringbuffer_.size()),
	channel_(nullptr) {

	readdone_.seq_ = 0;
	read_.seq_ = 1;
	writedone_.seq_ = 0;
	write_.seq_ = 1;
}

template<typename T>
void disruptor<T>::put(const T& val) {
	channel* ch = channel_.get();
	if (ch == nullptr) {
		ch = new channel();
		channel_.set(ch);
	}

	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		//等消费者消费
		klock<kmutex> lock(&ch->second);

		ringbuffer_[widx].first = ch;
		while (wseq > readdone_.seq_ + capacity_) {
			ch->first.wait(&ch->second);
		}
		ringbuffer_[widx].first = nullptr;
	}

	ringbuffer_[widx].second = val;

	while (wseq - 1 != writedone_.seq_) {
		//空转等待其它生产者生产
	}
	writedone_.seq_ = wseq;

	ch = ringbuffer_[widx].first;
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		if (ringbuffer_[widx].first == ch)
			ch->first.notify();
	}
}

template<typename T>
void disruptor<T>::put(T&& val) {
	channel* ch = channel_.get();
	if (ch == nullptr) {
		ch = new channel();
		channel_.set(ch);
	}

	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		//等消费者消费
		klock<kmutex> lock(&ch->second);

		ringbuffer_[widx].first = ch;
		while (wseq > readdone_.seq_ + capacity_) {
			ch->first.wait(&ch->second);
		}
		ringbuffer_[widx].first = nullptr;
	}

	ringbuffer_[widx].second = std::move(val);

	while (wseq - 1 != writedone_.seq_) {
		//空转等待其它生产者生产
	}
	writedone_.seq_ = wseq;

	ch = ringbuffer_[widx].first;
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		if (ringbuffer_[widx].first == ch)
			ch->first.notify();
	}
}

template<typename T>
void disruptor<T>::take(T& dst) {
	channel* ch = channel_.get();
	if (ch == nullptr) {
		ch = new channel();
		channel_.set(ch);
	}

	uint64_t rseq = read_.seq_++;
	uint64_t ridx = (rseq - 1) % capacity_;

	if (rseq > writedone_.seq_) {
		//等生产者生产
		klock<kmutex> lock(&ch->second);

		ringbuffer_[ridx].first = ch;
		while (rseq > writedone_.seq_) {
			ch->first.wait(&ch->second);
		}
		ringbuffer_[ridx].first = nullptr;
	}

	dst = ringbuffer_[ridx].second;

	while (rseq - 1 != readdone_.seq_) {
		//空转等待其它消费者消费
	}
	readdone_.seq_ = rseq;

	ch = ringbuffer_[ridx].first;
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		if (ringbuffer_[ridx].first == ch)
			ch->first.notify();
	}
}