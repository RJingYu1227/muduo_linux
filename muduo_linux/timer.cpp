#include"timer.h"

#include<string>

timer::timer(const functor &cb, int64_t time, double seconds)
	:Functor(cb),
	time_(time) {

	useconds_ = static_cast<int64_t>(seconds * 1000000);
	repeat_ = (useconds_ > 0);
}

/* (s) %Y%m%d-%H%M%S */
std::string timer::timeToString(time_t time) {
	char timebuf[32];
	tm tm_;
	localtime_r(&time, &tm_);//该函数是可重入的
	strftime(timebuf, sizeof timebuf, "%Y%m%d-%H%M%S", &tm_);
	return timebuf;
}

int64_t timer::getMicroUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
