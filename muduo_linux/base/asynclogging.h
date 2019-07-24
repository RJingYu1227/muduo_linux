#pragma once

#include"logstream.h"
#include"blockqueue.h"

class asynclogging :uncopyable {
public:

	asynclogging(const char* basename, off_t rollsize);
	~asynclogging() {
		if (running_)
			stop();
	}

	//kthread不会重复start和未开始就join
	void start() {
		running_ = 1;
		thread_.start();
	}
	void stop();

	void append(const char* data, size_t len);

private:
	typedef logbuffer<logstream::kLargeBuffer> buffer;
	typedef blockqueue<buffer*> buffer_queue;
	typedef std::pair<buffer_queue*, buffer*> entry;

	static thread_local buffer_queue thread_buffers_;
	static blockqueue<entry> async_queue_;

	void threadFunc();

	std::string basename_;
	off_t rollsize_;
	//int flush_interval_;

	kthread thread_;
	bool running_;

};
