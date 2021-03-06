﻿#pragma once

#include<pax/base/sharedatomic.h>
#include<pax/base/thread.h>

#include<pax/log/logstream.h>

#include<vector>

namespace pax {

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

	sharedatomic<uint64_t> readdone_;//已读末尾下标
	sharedatomic<uint64_t> read_;//可读起始下标
	sharedatomic<uint64_t> writedone_;//已写末尾下标
	sharedatomic<uint64_t> write_;//可写起始下标

	std::vector<char> ringbuffer_;

	std::string basename_;
	off_t rollsize_;

	mutex mutex_;
	cond cond_;
	thread thread_;

	volatile bool running_;

};

}//namespace pax