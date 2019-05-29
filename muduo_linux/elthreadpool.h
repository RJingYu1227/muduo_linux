#pragma once

#include"eventloop.h"
#include"tcpserver.h"
#include<vector>

class eventloop;
class tcpserver;

class elthreadpool {
public:
	elthreadpool(eventloop* baseloop, int num = 0);
	~elthreadpool();

	eventloop* getBaseLoop() { return baseloop_; }
	eventloop* getLoop();
	void start();
	int loopNum() { return num_; }

private:

	//为了线程调用，应该是静态成员函数
	static void* loopThreadFunc(void* loop);

	bool start_;
	int num_;
	int index_;
	eventloop* baseloop_;
	std::vector<eventloop*> loops_;
	//std::vector<pthread_t> tids_;
};
