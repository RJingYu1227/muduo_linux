#include<pax/base/thread.h>

#include<assert.h>

using namespace::pax;

namespace {

const uint64_t kNanoSecondsPerSecond = 1000 * 1000 * 1000;

}

bool mutex::timedlock(double seconds) {
	assert(seconds > 0);

	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);

	uint64_t nanoseconds = static_cast<uint64_t>(seconds * kNanoSecondsPerSecond);
	tsc.tv_sec += static_cast<time_t>((tsc.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
	tsc.tv_nsec = static_cast<long>((tsc.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

	return pthread_mutex_timedlock(&lock_, &tsc) == 0;//!= ETIMEDOUT
}

bool cond::timedwait(mutex* lock, double seconds) {
	assert(seconds > 0);

	timespec tsc;
	clock_gettime(CLOCK_REALTIME, &tsc);

	uint64_t nanoseconds = static_cast<uint64_t>(seconds * kNanoSecondsPerSecond);
	tsc.tv_sec += static_cast<time_t>((tsc.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
	tsc.tv_nsec = static_cast<long>((tsc.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

	return pthread_cond_timedwait(&cond_, (pthread_mutex_t*)lock, &tsc) == 0;//!= ETIMEDOUT
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
	while (started_ != 1)
		pthread_yield();

}

void thread::start(const pthread_attr_t* attr) {
	if (started_)
		return;

	int ret = pthread_create(&tid_, attr, pthreadFunc, this);
	assert(ret == 0);
	while (started_ != 1)
		pthread_yield();

}

int thread::join(void** val) {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_join(tid_, val);
}

int thread::detach() {
	if (!started_ || !joinable_)
		return 0;

	joinable_ = 0;

	return pthread_detach(tid_);
}