#pragma once

#include<functional>

class ktimer {
	friend class timerqueue;
public:
	typedef std::function<void()> functor;

	ktimer(const functor& cb, uint64_t time, double seconds);
	ktimer(functor&& cb, uint64_t time, double seconds);
	~ktimer() {}

	uint64_t getBorn()const { return born_; }
	uint64_t getTime()const { return time_; }
	uint64_t getInterval()const { return interval_; }
	void run() { func(); }

private:

	functor func;
	const uint64_t born_;//us
	uint64_t time_;//us
	uint64_t interval_;//us

};

//防止ktimer被销毁
class ktimerid {
	friend class timerqueue;
public:

	ktimerid() :
		born_(0),
		ptr_(nullptr)
	{}

	ktimerid(uint64_t born, ktimer* ptr) :
		born_(born),
		ptr_(ptr)
	{}

private:

	uint64_t born_;//us
	ktimer* ptr_;

};
