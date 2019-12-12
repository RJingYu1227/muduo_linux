﻿#pragma once

#include"logstream.h"
#include"disruptor.h"

#include<vector>

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
	static asynclogger* instance_;

	void threadFunc();

	sequence readdone_;//已读末尾下标
	sequence read_;//可读起始下标
	sequence writedone_;//已写末尾下标
	sequence write_;//可写起始下标

	std::vector<char> ringbuffer_;
	size_t end_index_;

	std::string basename_;
	off_t rollsize_;

	kmutex mutex_;
	kcond cond_;
	kthread thread_;

	volatile bool running_;

};