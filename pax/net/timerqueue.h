#pragma once

#include<pax/base/uncopyable.h>

#include<pax/net/timer.h>
#include<pax/net/channel.h>

#include<functional>
#include<set>
#include<vector>

namespace pax {

class eventloop;

class timerqueue :uncopyable {
public:
	typedef std::function<void()> functor;

	timerqueue(eventloop* loop);
	~timerqueue();

	ktimerid addTimer(const functor& func, uint64_t time, double seconds);
	ktimerid addTimer(functor&& func, uint64_t time, double seconds);

	void cancelTimer(ktimerid timer1);

private:
	typedef std::pair<uint64_t, timer*> entry;
	typedef std::set<entry> timer_set;

	//为了尽快处理handleRead
	void addTimerInLoop(timer* timer1);
	void cancelTimerInLoop(ktimerid timer1);

	void handleRead();
	void setExpireTimers(uint64_t now);
	void resetTimerfd(uint64_t time);

	int fd_;
	eventloop* loop_;
	channel channel_;

	//保存绝对到期时间
	timer_set timers_;
	timer_set cancel_timers_;
	std::vector<timer*> expire_timers_;

};

}//namespace pax