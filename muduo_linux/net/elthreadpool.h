#pragma once

#include"base/thread.h"

#include<vector>
#include<atomic>

namespace pax {

class eventloop;

class elthreadpool :uncopyable {
public:

	elthreadpool(int num);
	~elthreadpool() { stop(); }

	eventloop* getLoop();
	std::vector<eventloop*> getLoops()
	{
		return loops_;
	}

	void start();
	//在非线程池内线程调用；不允许通过其它方式使loop退出循环
	void stop();

	void setLoopNum(int num)
	{
		loop_num_ = num;
	}
	int getLoopNum()
	{
		return loop_num_;
	}

private:

	void threadFunc();

	mutex lock_;
	bool started_;
	int loop_num_;
	int index_;
	std::atomic_int32_t current_num_;
	std::vector<eventloop*> loops_;

};

}//namespace pax