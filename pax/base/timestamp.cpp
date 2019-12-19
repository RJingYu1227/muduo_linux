#include<pax/base/timestamp.h>

#include<sys/time.h>

using namespace::pax;

uint64_t timestamp::getMicroSeconds() {
	timeval tv = { 0 };
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000000 + tv.tv_usec;
}

uint64_t timestamp::getMillSeconds() {
	timeval tv = { 0 };
	gettimeofday(&tv, NULL);
	uint64_t ms = tv.tv_sec;
	ms *= 1000;
	ms += tv.tv_usec / 1000;

	return ms;
}

std::string timestamp::toFormattedString(bool us)const {
	char timebuf[64];
	tm tm_time;
	time_t seconds = static_cast<time_t>(micro_seconds_ / 1000000);
	localtime_r(&seconds, &tm_time);//该函数是可重入的

	if (us) {
		int microseconds = static_cast<int>(micro_seconds_ % 1000000);
		snprintf(timebuf, sizeof timebuf, "%4d %02d %02d %02d:%02d:%02d.%06d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
			microseconds);
	}
	else
		strftime(timebuf, sizeof timebuf, "%Y %m %d %H:%M:%S", &tm_time);

	return timebuf;
}