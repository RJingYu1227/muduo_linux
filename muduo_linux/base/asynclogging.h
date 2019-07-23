#pragma once

#include"logstream.h"
#include"blockqueue.h"

class asynclogging :uncopyable {
public:

	asynclogging(const char* basename, off_t rollsize, int flush_interval = 3);
	~asynclogging() {
		if (running_)
			stop();
	}

	//kthread不会重复start和未开始就join
	void start() {
		running_ = 1;
		thread_.start();
	}
	void stop() {
		running_ = 0;
		cond_.notify();
		thread_.join();
	}
	void append(const char* data, size_t len);

private:
	typedef logbuffer<logstream::kLargeBuffer> buffer;
	typedef blockqueue<buffer*> buffer_queue;
	typedef std::pair<buffer_queue*, buffer*> entry;

	void threadFunc();

	std::string basename_;
	off_t rollsize_;
	int flush_interval_;

	kmutex lock_;
	kcond cond_;
	kthread thread_;

	blockqueue<entry> async_buffers_;

	bool running_;

};
