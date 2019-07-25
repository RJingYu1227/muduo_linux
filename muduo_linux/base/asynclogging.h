#pragma once

#include"logstream.h"
#include"blockqueue.h"

class logfile;

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

	void append(const logstream::s_logbuffer& sbuff);

private:
	typedef logbuffer<logstream::kLargeBuffer> buffer;
	typedef blockqueue<buffer*> buffer_queue;

	struct impl {
		impl(buffer* buffer, buffer_queue* queue, logfile* output) :
			buffer_(buffer),
			queue_(queue),
			output_(output)
		{}
		
		buffer* buffer_ = nullptr;
		buffer_queue* queue_ = nullptr;
		logfile* output_ = nullptr;
	};

	static blockqueue<impl> async_buffers_;

	void threadFunc();

	std::string basename_;
	off_t rollsize_;

	kthread thread_;
	bool running_;

};