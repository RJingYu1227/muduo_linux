#include"timer.h"
#include<string>
#include<sys/time.h>
#include<unistd.h>

timer::timer(const event_callback &cb, int64_t time, double seconds) {
	Callback = cb;
	time_ = time;
	useconds_ = static_cast<int64_t>(seconds * 1000000);
	repeat_ = (useconds_ > 0);
}

std::string timer::timeToString(int64_t time_) {
	char buf[64] = { 0 };
	time_t seconds = static_cast<time_t>(time_ / 1000000);
	tm tm_time;
	gmtime_r(&seconds, &tm_time);
	snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
		tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
		tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
	return buf;
}

int64_t timer::getMicroUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}