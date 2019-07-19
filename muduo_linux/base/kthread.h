#pragma once

#include"uncopyable.h"

#include<pthread.h>
#include<functional>

template<typename T>
class klock {
public:

	klock(T* lock)
		:lock_(lock) {
		lock_->lock();
	}

	~klock() {
		lock_->unlock();
	}

private:

	T* lock_;

};

class kmutex {
public:

	kmutex() 
		:lock_(PTHREAD_MUTEX_INITIALIZER) {

	}
	~kmutex() {}

	void lock() { pthread_mutex_lock(&lock_); }
	void unlock() { pthread_mutex_unlock(&lock_); }
	bool trylock() { return pthread_mutex_trylock(&lock_); }
	bool timedlock(int seconds);

private:

	pthread_mutex_t lock_;

};

class kspin {
public:

	kspin() {
		pthread_spin_init(&lock_, 0);
	}
	~kspin() {}

	void lock() { pthread_spin_lock(&lock_); }
	void unlock() { pthread_spin_unlock(&lock_); }
	bool trylock() { return pthread_spin_trylock(&lock_); }

private:

	pthread_spinlock_t lock_;
};

class kcond {
public:

	kcond()
		:cond_(PTHREAD_COND_INITIALIZER) {

	}
	~kcond() {}

	void wait(kmutex* lock) { pthread_cond_wait(&cond_, (pthread_mutex_t*)lock); }
	void timedwait(kmutex* lock, int seconds);

	void notify() { pthread_cond_signal(&cond_); }
	void notifyAll() { pthread_cond_broadcast(&cond_); }

private:

	pthread_cond_t cond_;

};

class kthread :uncopyable {
public:
	typedef std::function<void()> functor;

	explicit kthread(const functor& func)
		:threadFunc(func),
		tid_(0),
		started_(0),
		joined_(0) {

	}
	explicit kthread(functor&& func)
		:threadFunc(std::move(func)),
		tid_(0),
		started_(0),
		joined_(0) {

	}
	~kthread() {
		if (started_ && !joined_)
			pthread_detach(tid_);
	}

	void start();
	void start(const pthread_attr_t* attr);
	int join();

	bool started()const { return started_; }
	pthread_t getTid()const { return tid_; }

private:

	static void* pthreadFunc(void* arg);

	functor threadFunc;
	pthread_t tid_;
	bool started_;
	bool joined_;

};