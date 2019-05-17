#pragma once

#include"channel.h"
#include"eventloop.h"
#include<functional>
#include<set>
#include<vector>
//#include<memory>

class eventloop;
class channel;

typedef std::function<void()> event_callback;

class timer {
public:
	timer(const event_callback &cb, int64_t time) {
		Callback = cb;
		time_ = time;
	}
	~timer() {}

	int64_t getTime() { return time_; }
	void run() { Callback(); }
private:
	event_callback Callback;
	int64_t time_;
};


class timerqueue
{
public:
	timerqueue(eventloop* loop);
	~timerqueue();

	timer* addTimer(const event_callback& cb, int64_t time);
	void cancelTimer(timer* timer1);

private:
	typedef std::pair<int64_t, timer*> entry;
	typedef std::set<entry> timer_list;
	typedef std::vector<entry> entry_vec;


	void addTimerInLoop(timer* timer1);
	void cancelTimerInLoop(timer* timer1);

	void handleRead();
	//void readTimerfd(int64_t now);
	bool insert(const entry& temp);
	void getTimers(int64_t now, entry_vec &temp);

	int64_t getUnixTime();
	void setTimespec(int64_t now, timespec& temp);
	void resetTimerfd(int64_t time);

	eventloop* loop_;
	int fd_;
	channel* channel_;
	
	timer_list timers_;
	timer_list active_timers_;

};

