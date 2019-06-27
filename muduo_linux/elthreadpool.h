#pragma once

#include"uncopyable.h"

#include<vector>

class eventloop;
class tcpserver;

class elthreadpool :uncopyable {
public:

	elthreadpool(int num);
	~elthreadpool() { stop(); }

	eventloop* getLoop();

	void start();
	void stop();

	void setLoopNum(int num) { num_ = num; }
	int getLoopNum() { return num_; }

private:

	//为了线程调用，应该是静态成员函数
	static void* threadFunc(void* loop);

	bool start_;
	int num_;
	int index_;
	std::vector<eventloop*> loops_;
	//std::vector<pthread_t> tids_;
};
