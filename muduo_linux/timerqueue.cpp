#include"timerqueue.h"
#include"eventloop.h"
#include"logging.h"

#include<sys/timerfd.h>
#include<unistd.h>
#include<strings.h>
#include<assert.h>

timerqueue::timerqueue(eventloop* loop)
	:loop_(loop),
	fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
	channel_(loop_, fd_) {

	assert(fd_ > 0);
	channel_.setReadCallback(std::bind(&timerqueue::handleRead, this));
	channel_.enableReading();
}


timerqueue::~timerqueue() {
	channel_.remove();
	close(fd_);
}

void timerqueue::addTimer(const event_callback& cb, int64_t time) {
	timer* temp_ = new timer(cb, time, 0);
	loop_->runInLoop(std::bind(&timerqueue::addTimerInLoop, this, temp_));
}

timer* timerqueue::addTimer(const event_callback &cb, int64_t time, double seconds) {
	timer* temp_ = new timer(cb, time, seconds);
	loop_->runInLoop(std::bind(&timerqueue::addTimerInLoop, this, temp_));
	return temp_;
}

void timerqueue::addTimerInLoop(timer* timer1) {
	int64_t time = timer1->time_;
	entry temp_(time, timer1);
	if (insert(temp_))
		resetTimerfd(time);
}

void timerqueue::cancelTimer(timer* timer1){
	loop_->runInLoop(std::bind(&timerqueue::cancelTimerInLoop, this, timer1));
}

void timerqueue::cancelTimerInLoop(timer* timer1) {
	entry temp_(timer1->time_, timer1);
	if (timers_.find(temp_) != timers_.end()) {
		timers_.erase(temp_);
		delete timer1;
	}
	else
		timer1->repeat_ = 0;
	//只有取消重复事件才是指针安全的，建议修改handleRead
	//也就是说一次性timer无法安全的取消
	//muduo解决的方式是加中间层
}

void timerqueue::handleRead() {
	int64_t now_;
	read(fd_, &now_, sizeof now_);
	now_ = timer::getMicroUnixTime();
	expire_timers_.clear();
	getTimers(now_);
	for (entry& ey : expire_timers_) {
		ey.second->run();
		if (ey.second->repeat_) {
			ey.second->restart(timer::getMicroUnixTime());
			ey.first = ey.second->time_;
			timers_.insert(ey);
		}
		else
			delete ey.second;
	}

	if (!timers_.empty()) {
		now_ = timers_.begin()->first;
		resetTimerfd(now_);
	}
}

bool timerqueue::insert(const timerqueue::entry &temp) {
	bool changed = 0;
	auto iter = timers_.begin();
	if (iter == timers_.end() || temp.first < iter->first)
		changed = 1;
	timers_.insert(temp);
	return changed;
}

void timerqueue::getTimers(int64_t now) {
	entry ey(now, reinterpret_cast<timer*>(UINTPTR_MAX));
	auto end = timers_.lower_bound(ey);
	std::copy(timers_.begin(), end, std::back_inserter(expire_timers_));
	timers_.erase(timers_.begin(),end);
}

void timerqueue::setTimespec(int64_t now, timespec& temp) {
	int64_t microseconds_ = now - timer::getMicroUnixTime();
	if (microseconds_ < 100)
		microseconds_ = 100;
	temp.tv_sec = microseconds_ / 1000000;
	temp.tv_nsec = (microseconds_ % 1000000) * 1000;
}

void timerqueue::resetTimerfd(int64_t time) {
	itimerspec new_value_;
	itimerspec old_value_;
	bzero(&new_value_, sizeof new_value_);
	setTimespec(time, new_value_.it_value);
	int ret = timerfd_settime(fd_, 0, &new_value_, &old_value_);
	if (ret)
		LOG << "定时器设置出错，errno = " << errno;
}

