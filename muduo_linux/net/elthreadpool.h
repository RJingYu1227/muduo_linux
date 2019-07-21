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
	std::vector<eventloop*> getLoops() 
	{ return loops_; }

	void start();
	void stop();//在非线程池内线程调用；会等待loop结束

	void setLoopNum(int num) 
	{ loop_num_ = num; }
	int getLoopNum() 
	{ return loop_num_; }

private:

	void threadFunc();

	bool started_;
	int loop_num_;
	int current_num_;//atmoic
	int index_;
	std::vector<eventloop*> loops_;

};
