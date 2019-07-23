#pragma once

#include"uncopyable.h"
#include"kthread.h"

#include<queue>

template<typename T>
class blockqueue :uncopyable {
public:
	void put_back(const T& val) {
		klock<kmutex> x(&lock_);
		queue_.push_back(val);
		cond_.notify();
	}
	void put_back(T&& val) {
		klock<kmutex> x(&lock_);
		queue_.push_back(std::move(val));
		cond_.notify();
	}
	bool tryput_back(const T& val);
	bool tryput_back(T&& val);
	T take_back();

	T take_front();
	bool tryput_front(T&& val);
	bool tryput_front(const T& val);
	void put_front(T&& val) {
		klock<kmutex> x(&lock_);
		queue_.push_front(std::move(val));
		cond_.notify();
	}
	void put_front(const T& val) {
		klock<kmutex> x(&lock_);
		queue_.push_front(val);
		cond_.notify();
	}

	bool empty()const {
		klock<kmutex> x(&lock_);
		return queue_.empty();
	}
	size_t size()const {
		klock<kmutex> x(&lock_);
		return queue_.size();
	}

private:

	kcond cond_;
	mutable kmutex lock_;
	std::deque<T> queue_;

};

/*
template<typename T>
void blockqueue<T>::put_back(const vector<T>& vec) {
	klock<kmutex> x(&lock_);
	for (const T& val : vec)
		queue_.push_back(val);
	cond_.notify();
}

template<typename T>
bool blockqueue<T>::tryput_back(const vector<T>& vec) {
	if (lock_.trylock()) {
		for (const T& val : vec)
			queue_.push_back(val);
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}
*/

template<typename T>
bool blockqueue<T>::tryput_back(const T& val) {
	if (lock_.trylock()) {
		queue_.push_back(val);
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
bool blockqueue<T>::tryput_back(T&& val) {
	if (lock_.trylock()) {
		queue_.push_back(std::move(val));
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
T blockqueue<T>::take_back() {
	klock<kmutex> x(&lock_);
	if (queue_.empty())
		cond_.wait(&lock_);
	T back(std::move(queue_.back()));
	queue_.pop_back();
	return std::move(back);
}

template<typename T>
T blockqueue<T>::take_front() {
	klock<kmutex> x(&lock_);
	if (queue_.empty())
		cond_.wait(&lock_);
	T front(std::move(queue_.front()));
	queue_.pop_front();
	return std::move(front);
}

template<typename T>
bool blockqueue<T>::tryput_front(T&& val) {
	if (lock_.trylock()) {
		queue_.push_front(std::move(val));
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
bool blockqueue<T>::tryput_front(const T& val) {
	if (lock_.trylock()) {
		queue_.push_front(val);
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}
