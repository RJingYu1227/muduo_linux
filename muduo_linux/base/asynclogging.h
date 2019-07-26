#pragma once

#include"kthread.h"
#include"logstream.h"

class logfile;

//可以考虑改成singleton模式
//因为实例本身并不包含append和threadFunc中的
//thread_local，未命名空间变量
class asynclogger :uncopyable {
public:

	asynclogger(const char* basename, off_t rollsize);
	~asynclogger() {
		if (running_)
			stop();
	}

	//kthread不会重复start和未开始就join
	void start() {
		running_ = 1;
		thread_.start();
	}
	void stop();

	void append(const s_logbuffer& sbuff);

private:

	void threadFunc();

	std::string basename_;
	off_t rollsize_;

	kthread thread_;
	bool running_;

};