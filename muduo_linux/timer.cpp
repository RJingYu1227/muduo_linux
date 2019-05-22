#include"timer.h"
#include<string>
#include<sys/time.h>

timer::timer(const event_callback &cb, int64_t time, double seconds) {
	Callback = cb;
	time_ = time;
	useconds_ = static_cast<int64_t>(seconds * 1000000);
	repeat_ = (useconds_ > 0);
}

std::string timer::timeToString(int64_t time) {
	time_t seconds = static_cast<time_t>(time / 1000000);
	return ctime(&seconds);
	//gmtime
	//localtime
	//ctime
	//asctime
}

int64_t timer::getMicroUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}