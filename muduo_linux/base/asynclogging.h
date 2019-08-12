#pragma once

#include"logstream.h"
#include"blockqueue.h"

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

	static asynclogger* instance_;

	void threadFunc();

	struct impl {
		impl(l_logbuffer* l_logbuffer, buffer_queue* queue, logfile* output) :
			buffer_(l_logbuffer),
			queue_(queue),
			output_(output)
		{}

		l_logbuffer* buffer_ = nullptr;
		buffer_queue* queue_ = nullptr;
		logfile* output_ = nullptr;
	};

	kthreadlocal<impl> thread_pimpl_;
	blockqueue<impl> async_impls_;
	std::string basename_;
	off_t rollsize_;
	kthread thread_;

	volatile bool running_;

};