#include"kthread.h"

#include<assert.h>

bool kmutex::timedlock(int seconds) {
	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);
	tsc.tv_sec += seconds;

	return pthread_mutex_timedlock(&lock_, &tsc);
}

void kcond::timedwait(kmutex* lock, int seconds) {
	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);
	tsc.tv_sec += seconds;

	pthread_cond_timedwait(&cond_, (pthread_mutex_t*)lock, &tsc);
}

void* kthread::pthreadFunc(void* arg) {
	kthread* temp = (kthread*)arg;
	functor func = std::move(temp->threadFunc);
	temp->started_ = 1;

	func();

	return (void*)0;
}

void kthread::start() {
	if (started_)
		return;

	int ret = pthread_create(&tid_, NULL, pthreadFunc, this);
	assert(ret == 0);
	while (started_ != 1);

}

void kthread::start(const pthread_attr_t* attr) {
	if (started_)
		return;

	int ret = pthread_create(&tid_, attr, pthreadFunc, this);
	assert(ret == 0);
	while (started_ != 1);

}

int kthread::join() {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_join(tid_, NULL);
}

int kthread::detach() {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_detach(tid_);
}