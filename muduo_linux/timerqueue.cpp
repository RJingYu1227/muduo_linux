#include "timerqueue.h"
#include<sys/timerfd.h>
#include<sys/time.h>
#include<unistd.h>
#include<strings.h>
#include<assert.h>

timerqueue::timerqueue(eventloop* loop) {
	loop_ = loop;
	fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	assert(fd_ > 0);
	lock_ = PTHREAD_MUTEX_INITIALIZER;
	channel_ = new channel(loop_, fd_);
	channel_->setReadCallback(std::bind(&timerqueue::handleRead, this));
	channel_->enableReading();
	//mpool_ = new memorypool<timer>();
}


timerqueue::~timerqueue() {
	channel_->remove();
	delete channel_;
	//delete mpool_;
	close(fd_);
}

timer* timerqueue::addTimer(const event_callback &cb, int64_t time, double seconds) {
	//pthread_mutex_lock(&lock_);

	//mpool_->setPtr(temp_);

	//pthread_mutex_unlock(&lock_);
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
	//pthread_mutex_lock(&lock_);

	//mpool_->destroyPtr(timer1);

	//pthread_mutex_unlock(&lock_);
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
}

void timerqueue::handleRead() {
	int64_t now_;
	read(fd_, &now_, sizeof now_);
	now_ = getMicroUnixTime();
	entry_vec temp_;
	getTimers(now_, temp_);
	for (entry& ey : temp_) {
		ey.second->run();
		if (ey.second->repeat_) {
			ey.second->restart(getMicroUnixTime());
			ey.first = ey.second->time_;
			timers_.insert(ey);
		}
		else {
			//pthread_mutex_lock(&lock_);

			//mpool_->destroyPtr(ey.second);

			//pthread_mutex_unlock(&lock_);
			delete ey.second;
		}
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
	timers_.erase(timers_.begin(),end);
}

int64_t timerqueue::getMicroUnixTime() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void timerqueue::setTimespec(int64_t now, timespec& temp) {
	int64_t microseconds_ = now - getMicroUnixTime();
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

