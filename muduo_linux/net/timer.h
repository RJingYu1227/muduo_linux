#pragma once

#include<functional>

namespace pax {

class timer {
	friend class timerqueue;
public:
	typedef std::function<void()> functor;

	timer(const functor& cb, uint64_t time, double seconds);
	timer(functor&& cb, uint64_t time, double seconds);
	~timer() {}

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

//防止timer被销毁
class ktimerid {
	friend class timerqueue;
public:

	ktimerid() :
		born_(0),
		ptr_(nullptr)
	{}

	ktimerid(uint64_t born, timer* ptr) :
		born_(born),
		ptr_(ptr)
	{}

private:

	uint64_t born_;//us
	timer* ptr_;

};

}//namespace pax