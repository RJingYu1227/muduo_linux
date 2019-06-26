#pragma once

#include"logstream.h"
#include<pthread.h>
#include<vector>
#include<string>
#include<atomic>
#include<time.h>

template class logbuffer<logstream::kLargeBuffer>;

class asynclogging {
public:

	asynclogging(const char* basename, off_t rollsize, int flush_interval = 3);
	~asynclogging() {
		if (running_)
			stop();
		delete buffer1_;
		delete buffer2_;
	}

	void start() {
		running_ = 1;
		threadFunc();
	}
	void append(const char* data, size_t len);
	void stop() {
		running_ = 0;
		pthread_cond_signal(&cond_);
		pthread_join(tid_, NULL);
	}

private:
	typedef logbuffer<logstream::kLargeBuffer> buffer;
	typedef std::vector<buffer*> buffer_vec;

	void threadFunc();

	std::string basename_;
	off_t rollsize_;
	int flush_interval_;

	pthread_t tid_;
	pthread_mutex_t lock_;
	pthread_cond_t cond_;

	buffer* buffer1_;
	buffer* buffer2_;
	buffer_vec buffers_;

	timespec time_;

	std::atomic<bool> running_;
};
