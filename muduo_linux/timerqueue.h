#pragma once

#include"channel.h"
#include"eventloop.h"
#include"memorypool.h"
#include<memory>
#include<functional>
#include<set>
#include<vector>

class eventloop;
class channel;

typedef std::function<void()> event_callback;

class timer {
	friend class timerqueue;
public:
	timer(const event_callback &cb, int64_t time, double seconds) {
		Callback = cb;
		time_ = time;
		useconds_ = static_cast<int64_t>(seconds * 1000000);
		repeat_ = (useconds_ > 0);
	}
	~timer() {}

	int64_t getTime() { return time_; }
	void run() { 
		Callback(); 
	}

private:
	void restart(int64_t now) { time_ = now + useconds_; }

	event_callback Callback;
	int64_t time_;
	int64_t useconds_;
	bool repeat_;
	//bool handling_;
};

//template class memorypool<timer>;

class timerqueue
{
public:
	timerqueue(eventloop* loop);
	~timerqueue();

	timer* addTimer(const event_callback& cb, int64_t time, double seconds = 0.0);
	void cancelTimer(timer* timer1);//只限于取消重复事件
	static int64_t getMicroUnixTime();//微秒为单位

private:
	typedef std::pair<int64_t, timer*> entry;
	typedef std::set<entry> timer_set;

	void addTimerInLoop(timer* timer1);
	void cancelTimerInLoop(timer* timer1);

	void handleRead();
	//void readTimerfd(int64_t now);
	bool insert(const entry& temp);
	void getTimers(int64_t now);

	void setTimespec(int64_t now, timespec& temp);
	void resetTimerfd(int64_t time);

	eventloop* loop_;
	int fd_;
	channel* channel_;
	//pthread_mutex_t lock_;
	//memorypool<timer>* mpool_;//线程不安全，考虑使用锁的性能再说
	
	timer_set timers_;
	std::vector<entry> expire_timers_;

};

