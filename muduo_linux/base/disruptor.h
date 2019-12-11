#pragma once

#include"kthread.h"

#include<atomic>
#include<vector>

struct sequence {
	enum cache {
		line_size = 64,
		padding_size = line_size - sizeof(std::atomic_uint64_t)
	};

	char front_[padding_size];
	std::atomic_uint64_t seq_;
	char back_[padding_size];

};

/*
生产者或消费者的数目不能大于capacity；
否则后果是不可预料的，全看操作系统对线程的调度；
当队列已满时，put会阻塞；队列已空时，take会阻塞
*/

template<typename T>
class disruptor :uncopyable {
public:

	explicit disruptor(size_t capacity);

	void put(const T& val);
	void put(T&& val);
	void take(T& dst);

private:
	typedef std::pair<kcond, kmutex> channel;

	sequence readdone_;//已读末尾下标
	sequence read_;//可读起始下标
	sequence writedone_;//已写末尾下标
	sequence write_;//可写起始下标

	size_t capacity_;
	std::vector<T> ringbuffer_;

	std::vector<channel*> channels_;
	kthreadlocal<channel> thread_channel_;

};

template<typename T>
disruptor<T>::disruptor(size_t capacity) :
	capacity_(capacity),
	ringbuffer_(capacity),
	channels_(capacity_),
	thread_channel_(kthreadlocal<channel>::freeFunc) {

	readdone_.seq_ = 0;
	read_.seq_ = 1;
	writedone_.seq_ = 0;
	write_.seq_ = 1;
}

template<typename T>
void disruptor<T>::put(const T& val) {
	channel* ch;
	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		ch = thread_channel_.get();
		if (ch == nullptr) {
			ch = new channel();
			thread_channel_.set(ch);
		}

		klock<kmutex> lock(&ch->second);

		channels_[widx] = ch;
		while (wseq > readdone_.seq_ + capacity_) {
			ch->first.wait(&ch->second);
		}
		channels_[widx] = nullptr;
	}

	ringbuffer_[widx] = val;

	while (wseq - 1 != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	ch = channels_[widx];
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		if (channels_[widx] == ch)
			ch->first.notify();
	}
}

template<typename T>
void disruptor<T>::put(T&& val) {
	channel* ch;
	uint64_t wseq = write_.seq_++;
	uint64_t widx = (wseq - 1) % capacity_;

	if (wseq > readdone_.seq_ + capacity_) {
		ch = thread_channel_.get();
		if (ch == nullptr) {
			ch = new channel();
			thread_channel_.set(ch);
		}

		klock<kmutex> lock(&ch->second);

		channels_[widx] = ch;
		while (wseq > readdone_.seq_ + capacity_) {
			ch->first.wait(&ch->second);
		}
		channels_[widx] = nullptr;
	}

	ringbuffer_[widx] = std::move(val);

	while (wseq - 1 != writedone_.seq_) {
		pthread_yield();
	}
	writedone_.seq_ = wseq;

	ch = channels_[widx];
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		if (channels_[widx] == ch)
			ch->first.notify();
	}
}

template<typename T>
void disruptor<T>::take(T& dst) {
	channel* ch;
	uint64_t rseq = read_.seq_++;
	uint64_t ridx = (rseq - 1) % capacity_;

	if (rseq > writedone_.seq_) {
		//等生产者生产
		ch = thread_channel_.get();
		if (ch == nullptr) {
			ch = new channel();
			thread_channel_.set(ch);
		}

		klock<kmutex> lock(&ch->second);

		channels_[ridx] = ch;
		while (rseq > writedone_.seq_) {
			ch->first.wait(&ch->second);
			//虚假唤醒，以及特殊情况下被上一次的生产者唤醒
		}
		channels_[ridx] = nullptr;
	}

	dst = ringbuffer_[ridx];

	while (rseq - 1 != readdone_.seq_) {
		//等待其它消费者消费
		pthread_yield();
	}
	readdone_.seq_ = rseq;

	ch = channels_[ridx];
	if (ch != nullptr) {
		klock<kmutex> lock(&ch->second);
		//此时生产者阻塞在wait或者已经可以进行生产
		//特殊情况下，同一个生产者又再次阻塞在wait，可以视作一次虚假唤醒
		if (channels_[ridx] == ch)
			ch->first.notify();
	}
}