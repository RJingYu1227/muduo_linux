#pragma once

#include"channel.h"
#include"eventloop.h"
#include"timer.h"
#include"uncopyable.h"

#include<memory>
#include<functional>
#include<set>
#include<vector>

class eventloop;
class channel;
class timer;

class timerqueue :uncopyable {
public:
	typedef std::function<void()> event_callback;

	timerqueue(eventloop* loop);
	~timerqueue();

	timer* addTimer(const event_callback& cb, int64_t time, double seconds = 0.0);
	void cancelTimer(timer* timer1);//只限于取消重复事件

private:
	typedef std::pair<int64_t, timer*> entry;
	typedef std::set<entry> timer_set;

	//为了尽快处理handleRead
	void addTimerInLoop(timer* timer1);
	void cancelTimerInLoop(timer* timer1);

	void handleRead();
	bool insert(const entry& temp);
	void getTimers(int64_t now);

	void setTimespec(int64_t now, timespec& temp);
	void resetTimerfd(int64_t time);

	eventloop* loop_;
	int fd_;
	channel* channel_;
	
	timer_set timers_;
	std::vector<entry> expire_timers_;

};

