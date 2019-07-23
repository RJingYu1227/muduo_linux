#pragma once

#include"uncopyable.h"
#include"kthread.h"

#include<queue>
#include<vector>

using std::vector;

template<typename T>
class blockqueue :uncopyable {
public:

	void put(const vector<T>& vec);
	bool tryput(const vector<T>& vec);

	void put(const T& val) {
		klock<kmutex> x(&lock_);
		queue_.push(val);
		cond_.notify();
	}
	bool tryput(const T& val);

	void put(T&& val) {
		klock<kmutex> x(&lock_);
		queue_.push(std::move(val));
		cond_.notify();
	}
	bool tryput(T&& val);

	T take();

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
	std::queue<T> queue_;

};

template<typename T>
void blockqueue<T>::put(const vector<T>& vec) {
	klock<kmutex> x(&lock_);
	for (const T& val : vec)
		queue_.push(val);
	cond_.notify();
}

template<typename T>
bool blockqueue<T>::tryput(const vector<T>& vec) {
	if (lock_.trylock()) {
		for (const T& val : vec)
			queue_.push(val);
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
bool blockqueue<T>::tryput(const T& val) {
	if (lock_.trylock()) {
		queue_.push(val);
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
bool blockqueue<T>::tryput(T&& val) {
	if (lock_.trylock()) {
		queue_.push(std::move(val));
		cond_.notify();
		lock_.unlock();
		return 1;
	}
	return 0;
}

template<typename T>
T blockqueue<T>::take() {
	klock<kmutex> x(&lock_);
	if (queue_.empty())
		cond_.wait(&lock_);
	T front(std::move(queue_.front()));
	queue_.pop();
	return std::move(front);
}