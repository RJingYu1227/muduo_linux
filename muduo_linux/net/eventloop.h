#pragma once

#include"uncopyable.h"

#include<pthread.h>
#include<vector>
#include<functional>

class channel;
class poller;
class timer;
class timerqueue;
class eventqueue;

class eventloop :uncopyable {
public:
	typedef std::function<void()> functor;

	static eventloop* getEventLoop();

	eventloop();
	~eventloop();

	void updateThread();
	void loop();
	void quit();
	bool isLooping()const { return looping_; }
	void assertInLoopThread();
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	bool isInLoopThread()const; 

	void runInLoop(const functor& func);
	void queueInLoop(const functor& func);//const左值引用

	void runAt(const functor& cb, int64_t time);
	void runAfter(const functor& cb, double seconds);
	const timer* runEvery(const functor& cb, double seconds);
	void cancelTimer(timer* timer1);//一个timer只能调用一次

private:

	void doFunctors();

	pthread_t tid_;//第一个初始化
	bool quit_;
	bool looping_;
	int timeout_;
	poller* poller_;
	eventqueue* eventque_;//最后执行
	timerqueue* timerque_;//尽快执行
	std::vector<channel*> active_channels_;
	std::vector<functor> functors_;

};

