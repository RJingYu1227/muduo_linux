#include"kthread.h"

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