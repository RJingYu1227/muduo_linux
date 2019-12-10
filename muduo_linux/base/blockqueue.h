﻿#pragma once

#include"kthread.h"

#include<queue>

template<typename T>
class blockqueue :uncopyable {
public:

	size_t timedwait(int seconds);
	inline bool empty()const;
	inline size_t size()const;

	//可以考虑使用initializer_list实现一次put多个

	void put_front(const T& val);
	void put_front(T&& val);
	bool tryput_front(const T& val);
	T take_front();

	void put_back(const T& val);
	void put_back(T&& val);
	bool tryput_back(const T& val);
	T take_back();

private:

	kcond cond_;
	mutable kmutex lock_;
	std::deque<T> queue_;

};

template<typename T>
size_t blockqueue<T>::timedwait(int seconds) {
	klock<kmutex> x(&lock_);
	if (queue_.empty())//这里不用while
		cond_.timedwait(&lock_, seconds);
	return queue_.size();
}

template<typename T>
bool blockqueue<T>::empty()const {
	klock<kmutex> x(&lock_);
	return queue_.empty();
}

template<typename T>
size_t blockqueue<T>::size()const {
	klock<kmutex> x(&lock_);
	return queue_.size();
}

template<typename T>
void blockqueue<T>::put_front(const T& val) {
	klock<kmutex> x(&lock_);
	queue_.push_front(val);
	cond_.notify();
}

template<typename T>
void blockqueue<T>::put_front(T&& val) {
	klock<kmutex> x(&lock_);
	queue_.push_front(std::move(val));
	cond_.notify();
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

template<typename T>
T blockqueue<T>::take_front() {
	klock<kmutex> x(&lock_);
	while (queue_.empty())//虚假唤醒
		cond_.wait(&lock_);
	T front(std::move(queue_.front()));
	queue_.pop_front();
	return std::move(front);
}

template<typename T>
void blockqueue<T>::put_back(const T& val) {
	klock<kmutex> x(&lock_);
	queue_.push_back(val);
	cond_.notify();
}

template<typename T>
void blockqueue<T>::put_back(T&& val) {
	klock<kmutex> x(&lock_);
	queue_.push_back(std::move(val));
	cond_.notify();
}

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
T blockqueue<T>::take_back() {
	klock<kmutex> x(&lock_);
	while (queue_.empty())//虚假唤醒
		cond_.wait(&lock_);
	T back(std::move(queue_.back()));
	queue_.pop_back();
	return std::move(back);
}