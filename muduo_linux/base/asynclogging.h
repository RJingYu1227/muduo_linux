#pragma once

#include"uncopyable.h"
#include"logstream.h"
#include"blockqueue.h"

#include<vector>

class logfile;

class asynclogger :uncopyable {
public:

	static asynclogger* initialize(const char* basename, off_t rollsize) {
		static asynclogger instance(basename, rollsize);
		instance_ = &instance;
		return instance_;
	}

	static asynclogger* instance() { return instance_; }

	void start() {
		running_ = 1;
		thread_.start();
	}

	void append(const s_logbuffer& sbuff);

protected:

	asynclogger(const char* basename, off_t rollsize);
	~asynclogger();

private:
	typedef blockqueue<l_logbuffer*> buffer_queue;

	//static void implDestory(void* ptr);
	static asynclogger* instance_;

	void threadFunc();
	
	struct impl {
		impl(l_logbuffer* l_logbuffer, buffer_queue* queue, logfile* output) :
			buffer_(l_logbuffer),
			queue_(queue),
			output_(output)
		{}

		impl() {}

		l_logbuffer* buffer_ = nullptr;
		buffer_queue* queue_ = nullptr;
		logfile* output_ = nullptr;
	};

	kthreadlocal<impl> thread_pimpl_;
	blockqueue<impl> full_impls_;

	kmutex lock_;
	std::vector<impl> empty_impls_;

	std::string basename_;
	off_t rollsize_;
	kthread thread_;

	volatile bool running_;

};