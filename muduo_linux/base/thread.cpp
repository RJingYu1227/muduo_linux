#include"thread.h"

#include<assert.h>

using namespace::pax;

bool mutex::timedlock(int seconds) {
	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);
	tsc.tv_sec += seconds;

	return pthread_mutex_timedlock(&lock_, &tsc);
}

void cond::timedwait(mutex* lock, int seconds) {
	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);
	tsc.tv_sec += seconds;

	pthread_cond_timedwait(&cond_, (pthread_mutex_t*)lock, &tsc);
}

void* thread::pthreadFunc(void* arg) {
	thread* temp = (thread*)arg;
	functor func = std::move(temp->threadFunc);
	temp->started_ = 1;

	func();

	return (void*)0;
}

void thread::start() {
	if (started_)
		return;

	int ret = pthread_create(&tid_, NULL, pthreadFunc, this);
	assert(ret == 0);
	while (started_ != 1);

}

void thread::start(const pthread_attr_t* attr) {
	if (started_)
		return;

	int ret = pthread_create(&tid_, attr, pthreadFunc, this);
	assert(ret == 0);
	while (started_ != 1);

}

int thread::join() {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_join(tid_, NULL);
}

int thread::detach() {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_detach(tid_);
}