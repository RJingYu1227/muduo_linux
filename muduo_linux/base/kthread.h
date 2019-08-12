#pragma once

#include"uncopyable.h"

#include<pthread.h>
#include<functional>

template<typename T>
class kthreadlocal {
public:

	kthreadlocal() {
		pthread_key_create(&key_, NULL);
	}

	~kthreadlocal() {
		pthread_key_delete(key_);
	}

	void set(const T* ptr) { pthread_setspecific(key_, ptr); }
	T* get() { return (T*)pthread_getspecific(key_); }

private:

	pthread_key_t key_;

};

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
	bool trylock() { return pthread_mutex_trylock(&lock_) == 0; }
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
	bool trylock() { return pthread_spin_trylock(&lock_) == 0; }

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
		joinable_(1) {

	}
	explicit kthread(functor&& func)
		:threadFunc(std::move(func)),
		tid_(0),
		started_(0),
		joinable_(1) {

	}
	~kthread() {
		detach();
	}

	void start();
	void start(const pthread_attr_t* attr);
	int join();
	int detach();

	bool joinable()const { return joinable_; }
	bool started()const { return started_; }
	pthread_t getTid()const { return tid_; }

private:

	static void* pthreadFunc(void* arg);

	functor threadFunc;
	pthread_t tid_;
	bool started_;
	bool joinable_;

};