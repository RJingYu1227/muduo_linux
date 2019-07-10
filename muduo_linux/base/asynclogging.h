﻿#pragma once

#include"logstream.h"
#include"kthread.h"
#include"uncopyable.h"

#include<vector>
#include<string>

class asynclogging :uncopyable {
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
		cond_.notify();
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
	kmutex lock_;
	kcond cond_;

	buffer* buffer1_;
	buffer* buffer2_;
	buffer_vec buffers_;

	bool running_;

};
