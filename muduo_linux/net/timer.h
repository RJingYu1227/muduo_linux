#pragma once

#include<functional>
#include<sys/time.h>

class timer {
	friend class timerqueue;
public:
	typedef std::function<void()> functor;

	timer(const functor &cb, int64_t time, double seconds);
	timer(functor&& cb, int64_t time, double seconds);
	~timer() {}

	int64_t getTime()const { return time_; }
	void run() { Functor(); }

	static std::string timeToString(time_t time);
	static int64_t getMicroUnixTime();
	static time_t getUnixTime() { return time(NULL); }

private:

	void restart(int64_t now) { time_ = now + useconds_; }

	functor Functor;
	int64_t time_;
	int64_t useconds_;
	bool repeat_;

};
