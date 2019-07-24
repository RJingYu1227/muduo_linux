#include"ktimer.h"

#include<string>

ktimer::ktimer(const functor &cb, int64_t time, double seconds)
	:func(cb),
	born_(time),
	time_(time),
	interval_(static_cast<int64_t>(seconds * 1000000)) {

}

ktimer::ktimer(functor&& cb, int64_t time, double seconds)
	:func(std::move(cb)),
	born_(time),
	time_(time),
	interval_(static_cast<int64_t>(seconds * 1000000)) {

}

/* (s) %Y/%m/%d-%H:%M:%S */
std::string ktimer::timeToString(time_t time) {
	char timebuf[32];
	tm tm_;
	localtime_r(&time, &tm_);//该函数是可重入的
	strftime(timebuf, sizeof timebuf, "%Y/%m/%d-%H:%M:%S", &tm_);
	return timebuf;
}

int64_t ktimer::getMicroUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
