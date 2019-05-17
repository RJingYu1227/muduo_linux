#include "timerqueue.h"
#include<sys/timerfd.h>
#include<sys/time.h>
#include<unistd.h>
#include<strings.h>
#include<assert.h>

timerqueue::timerqueue(eventloop* loop)
	:loop_(loop) {
	fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	assert(fd_ > 0);
	channel_ = new channel(loop_, fd_);
	channel_->setReadCallback(std::bind(&timerqueue::handleRead, this));
	channel_->enableReading();
}


timerqueue::~timerqueue() {
	channel_->remove();
	delete channel_;
	close(fd_);
	for (const entry &timer1 : timers_)
		delete timer1.second;
}

timer* timerqueue::addTimer(const event_callback &cb, int64_t time) {
	timer* timer1 = new timer(cb, time);
	loop_->runInLoop(std::bind(&timerqueue::addTimerInLoop, this, timer1));
	return timer1;
}

void timerqueue::addTimerInLoop(timer* timer1) {
	int64_t time_ = timer1->getTime();
	entry temp_(time_, timer1);
	if (insert(temp_))
		resetTimerfd(time_);

}

void timerqueue::cancelTimer(timer* timer1){
	loop_->runInLoop(std::bind(&timerqueue::cancelTimerInLoop, this, timer1));
}

void timerqueue::cancelTimerInLoop(timer* timer1) {
	entry temp_(timer1->getTime(), timer1);
	timers_.erase(temp_);
}

void timerqueue::handleRead() {
	int64_t now_;
	read(fd_, &now_, sizeof now_);
	now_ = getUnixTime();
	entry_vec temp_;
	getTimers(now_, temp_);
	for (const entry& timer_ : temp_) {
		timer_.second->run();
		delete timer_.second;
		timers_.erase(timer_);
	}

	if (!timers_.empty()) {
		now_ = timers_.begin()->first;
		resetTimerfd(now_);
	}
}

bool timerqueue::insert(const timerqueue::entry &temp) {
	bool changed = 0;
	timer_list::iterator iter = timers_.begin();
	if (iter == timers_.end() || temp.first < iter->first)
		changed = 1;
	timers_.insert(temp);
	return changed;
}

void timerqueue::getTimers(int64_t now, timerqueue::entry_vec& temp) {
	entry ey(now, reinterpret_cast<timer*>(UINTPTR_MAX));
	timer_list::iterator end = timers_.lower_bound(ey);
	std::copy(timers_.begin(), end, std::back_inserter(temp));
}

int64_t timerqueue::getUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void timerqueue::setTimespec(int64_t now, timespec& temp) {
	int64_t microseconds_ = now - getUnixTime();
	if (microseconds_ < 100)
		microseconds_ = 100;
	temp.tv_sec = microseconds_ / 1000000;
	temp.tv_nsec = (microseconds_ % 1000000) * 1000;
}

void timerqueue::resetTimerfd(int64_t time) {
	itimerspec new_value_;
	itimerspec old_value_;
	bzero(&new_value_, sizeof new_value_);
	//bzero(&old_value_, sizeof old_value_);
	setTimespec(time, new_value_.it_value);
	int ret = timerfd_settime(fd_, 0, &new_value_, &old_value_);
	if (ret)
		perror("定时器设置出错");
}

