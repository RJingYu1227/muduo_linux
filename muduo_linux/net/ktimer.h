#pragma once

#include<functional>
#include<sys/time.h>

class ktimer {
	friend class timerqueue;
public:
	typedef std::function<void()> functor;

	ktimer(const functor& cb, int64_t time, double seconds);
	ktimer(functor&& cb, int64_t time, double seconds);
	~ktimer() {}

	int64_t getBorn()const { return born_; }
	int64_t getTime()const { return time_; }
	int64_t getInterval()const { return interval_; }
	void run() { func(); }

	static std::string timeToString(time_t time);
	static int64_t getMicroUnixTime();
	static time_t getUnixTime() { return time(NULL); }

private:

	functor func;
	const int64_t born_;//us
	int64_t time_;//us
	int64_t interval_;//us

};

//防止ktimer被销毁
class ktimerid {
	friend class timerqueue;
public:

	ktimerid() :
		born_(0),
		ptr_(nullptr)
	{}

	ktimerid(int64_t born, ktimer* ptr) :
		born_(born),
		ptr_(ptr)
	{}

private:

	int64_t born_;//us
	ktimer* ptr_;

};
