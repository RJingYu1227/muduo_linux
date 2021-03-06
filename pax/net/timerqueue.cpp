﻿#include<pax/base/timestamp.h>

#include<pax/log/logging.h>

#include<pax/net/timerqueue.h>
#include<pax/net/eventloop.h>

#include<sys/timerfd.h>
#include<unistd.h>
#include<strings.h>
#include<assert.h>

using namespace::pax;

timerqueue::timerqueue(eventloop* loop)
	:fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
	loop_(loop),
	channel_(loop_, fd_) {

	assert(fd_ > 0);
	channel_.setReadCallback(std::bind(&timerqueue::handleRead, this));
	channel_.enableReading();
}


timerqueue::~timerqueue() {
	channel_.remove();
	close(fd_);
}

ktimerid timerqueue::addTimer(const functor& func, uint64_t time, double seconds) {
	timer* temp = new timer(func, time, seconds);
	loop_->runInLoop(std::bind(&timerqueue::addTimerInLoop, this, temp));

	return ktimerid(time, temp);
}

ktimerid timerqueue::addTimer(functor&& func, uint64_t time, double seconds) {
	timer* temp = new timer(std::move(func), time, seconds);
	loop_->runInLoop(std::bind(&timerqueue::addTimerInLoop, this, temp));

	return ktimerid(time, temp);
}

void timerqueue::addTimerInLoop(timer* timer) {
	auto iter = timers_.begin();
	if (iter == timers_.end() || timer->time_ < iter->first)
		resetTimerfd(timer->time_);

	timers_.emplace(timer->time_, timer);
}

void timerqueue::cancelTimer(ktimerid timerid){
	if (timerid.born_ == 0 || timerid.ptr_ == nullptr)
		return;

	loop_->runInLoop(std::bind(&timerqueue::cancelTimerInLoop, this, timerid));
}

void timerqueue::cancelTimerInLoop(ktimerid timerid) {
	//确保不会把正在处理或将要处理的timer销毁
	cancel_timers_.emplace(timerid.born_, timerid.ptr_);
}

void timerqueue::handleRead() {
	uint64_t now;
	read(fd_, &now, sizeof now);
	now = timestamp::getMicroSeconds();
	setExpireTimers(now);

	for (auto temp : expire_timers_) {
		temp->run();
		if (temp->interval_ > 0) {
			temp->time_ = timestamp::getMicroSeconds() + temp->interval_;
			timers_.emplace(temp->time_, temp);
			continue;
		}
		delete temp;
	}

	if (!timers_.empty()) {
		now = timers_.begin()->first;
		resetTimerfd(now);
	}
}

void timerqueue::setExpireTimers(uint64_t now) {
	expire_timers_.clear();
	entry temp(now, reinterpret_cast<timer*>(UINTPTR_MAX));
	auto iter = timers_.begin();
	auto end = timers_.lower_bound(temp);

	while (iter != end) {
		timer* x = iter->second;
		if (!cancel_timers_.erase(entry(x->born_, x)))
			expire_timers_.push_back(x);
		else
			delete x;
		++iter;
	}
	timers_.erase(timers_.begin(), end);
}

void timerqueue::resetTimerfd(uint64_t time) {
	itimerspec utmr;
	bzero(&utmr, sizeof utmr);

	uint64_t microseconds_ = time - timestamp::getMicroSeconds();
	if (microseconds_ < 100)
		microseconds_ = 100;
	utmr.it_value.tv_sec = microseconds_ / 1000000;
	utmr.it_value.tv_nsec = (microseconds_ % 1000000) * 1000;

	//设置相对到期时间
	int ret = timerfd_settime(fd_, 0, &utmr, NULL);
	if (ret)
		LOG << "定时器设置出错，errno = " << errno;
}