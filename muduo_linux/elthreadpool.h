#pragma once

#include"eventloop.h"
#include"tcpserver.h"
#include<vector>

class eventloop;
class tcpserver;

class elthreadpool {
public:
	elthreadpool(int num = 2);
	~elthreadpool();

	eventloop* getServerLoop() { return serverloop_; }
	eventloop* getIoLoop();
	void start();
	int loopNum() { return num_; }

private:
	//static void* serverThread(void* loop);//为了线程调用，应该是静态成员函数
	static void* ioThread(void* loop);

	bool start_;
	int num_;
	int index_;
	eventloop* serverloop_;
	std::vector<eventloop*> ioloops_;
};
