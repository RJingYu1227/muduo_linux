#pragma once

#include"uncopyable.h"

#include<vector>

class eventloop;
class tcpserver;

class elthreadpool :uncopyable {
public:

	elthreadpool(eventloop* baseloop, int num);
	~elthreadpool();

	eventloop* getBaseLoop() { return baseloop_; }
	eventloop* getLoop();

	void start();
	void stop();

	void setLoopNum(int num) { num_ = num; }
	int loopNum() { return num_; }

private:

	//为了线程调用，应该是静态成员函数
	static void* threadFunc(void* loop);

	bool start_;
	int num_;
	int index_;
	eventloop* baseloop_;
	std::vector<eventloop*> loops_;
	//std::vector<pthread_t> tids_;
};
