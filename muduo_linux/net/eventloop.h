#pragma once

#include"uncopyable.h"
#include"ktimer.h"

#include<vector>

class channel;
class poller;
class timerqueue;
class eventqueue;

class eventloop :uncopyable {
public:
	typedef std::function<void()> functor;

	static eventloop* getEventLoop();

	eventloop();
	~eventloop();

	void loop();
	void quit();
	bool isLooping()const { return looping_; }
	void assertInLoopThread();
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	bool isInLoopThread()const; 

	void runInLoop(const functor& func);
	void runInLoop(functor&& func);
	void queueInLoop(const functor& func);
	void queueInLoop(functor&& func);

	ktimerid runAt(const functor& func, int64_t time);
	ktimerid runAt(functor&& func, int64_t time);
	ktimerid runAfter(const functor& func, double seconds);
	ktimerid runAfter(functor&& func, double seconds);
	ktimerid runEvery(const functor& func, double seconds);
	ktimerid runEvery(functor&& func, double seconds);
	void cancelTimer(ktimerid timer1);

private:

	void doFunctors();

	unsigned long tid_;
	bool quit_;
	bool looping_;
	int timeout_;
	poller* poller_;
	eventqueue* eventque_;//最后执行
	timerqueue* timerque_;//尽快执行
	std::vector<channel*> active_channels_;
	std::vector<functor> functors_;

};

