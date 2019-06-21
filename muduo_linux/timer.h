#pragma once

#include<functional>

class timer {
	friend class timerqueue;
public:
	typedef std::function<void()> event_callback;

	timer(const event_callback &cb, int64_t time, double seconds);
	~timer() {}

	int64_t getTime() { return time_; }
	void run() { Callback(); }

	static std::string timeToString(int64_t time);
	static int64_t getMicroUnixTime();//微秒为单位

private:
	void restart(int64_t now) { time_ = now + useconds_; }

	event_callback Callback;
	int64_t time_;
	int64_t useconds_;
	bool repeat_;
	//bool handling_;
};
