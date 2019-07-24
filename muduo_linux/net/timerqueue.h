#pragma once

#include"ktimer.h"
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

	ktimerid addTimer(const functor& func, int64_t time, double seconds);
	ktimerid addTimer(functor&& func, int64_t time, double seconds);

	void cancelTimer(ktimerid timer1);

private:
	typedef std::pair<int64_t, ktimer*> entry;
	typedef std::set<entry> timer_set;

	//为了尽快处理handleRead
	void addTimerInLoop(ktimer* timer1);
	void cancelTimerInLoop(ktimerid timer1);

	void handleRead();
	void setExpireTimers(int64_t now);
	void resetTimerfd(int64_t time);

	int fd_;
	eventloop* loop_;
	channel channel_;
	
	//保存绝对到期时间
	timer_set timers_;
	timer_set cancel_timers_;
	std::vector<ktimer*> expire_timers_;

};

