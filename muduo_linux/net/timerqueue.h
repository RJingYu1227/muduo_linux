﻿#pragma once

#include"timer.h"
#include"channel.h"
#include"uncopyable.h"

#include<functional>
#include<set>
#include<vector>

class eventloop;

class timerqueue :uncopyable {
public:
	typedef std::function<void()> functor;

	timerqueue(eventloop* loop);
	~timerqueue();

	void addTimer(const functor& func, int64_t time);
	void addTimer(functor&& func, int64_t time);
	timer* addTimer(const functor& func, int64_t time, double seconds);
	timer* addTimer(functor&& func, int64_t time, double seconds);

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
	channel channel_;
	
	timer_set timers_;
	std::vector<entry> expire_timers_;

};
