#include"eventloop.h"
#include"logging.h"
#include"channel.h"
#include"poller.h"
#include"timerqueue.h"
#include"eventqueue.h"

#include<iostream>
#include<assert.h>
#include<unistd.h>

__thread eventloop* loop_inthisthread_ = nullptr;

eventloop* eventloop::getEventLoop() {
	return loop_inthisthread_;
}

void eventloop::updateThread() {
	tid_ = pthread_self();
	loop_inthisthread_ = this;
	LOG << "事件循环：" << loop_inthisthread_ << " 的新线程：" << tid_;
}

eventloop::eventloop() :
	tid_(pthread_self()),
	quit_(0),
	looping_(0),
	timeout_(-1),
	poller_(poller::newPoller(this)),
	eventque_(new eventqueue(this)),
	timerque_(new timerqueue(this)) {

	loop_inthisthread_ = this;
	LOG << "在线程：" << tid_ << " 创建事件循环：" << this;
}

eventloop::~eventloop() {
	assert(!looping_);
	delete timerque_;
	delete eventque_;
	delete poller_;
	loop_inthisthread_ = nullptr;

	LOG << "在线程：" << pthread_self() << " 关闭事件循环：" << this;
}

void eventloop::assertInLoopThread() {
	if (tid_ != pthread_self())
		LOG << "事件循环：" << this << " 属于线程：" << tid_ << " 目前线程为：" << pthread_self();
}

bool eventloop::isInLoopThread()const {
	return tid_ == pthread_self();
}

void eventloop::updateChannel(channel* ch) {
	assert(ch->getLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(ch);
}

void eventloop::removeChannel(channel* ch) {
	assert(ch->getLoop() == this);
	assertInLoopThread();
	poller_->removeChannel(ch);
}

void eventloop::loop() {
	assert(!looping_);
	assertInLoopThread();
	looping_ = 1;
	quit_ = 0;
	LOG << "事件循环：" << this << " 开始";

	while (!quit_) {
		active_channels_.clear();
		poller_->doPoll(timeout_, active_channels_);
		for (auto ch : active_channels_)
			ch->handleEvent();
		doFunctors();//注意这里的执行顺序
	}
	doFunctors();//注意这里

	LOG << "事件循环：" << this << " 停止";
	looping_ = 0;
}

void eventloop::quit() {
	if (quit_)
		return;
	quit_ = 1;

	if (!isInLoopThread())
		eventque_->wakeup();
}

void eventloop::runInLoop(const functor& func) {
	if (isInLoopThread())
		func();
	else
		queueInLoop(func);
}

//std::move没必要
void eventloop::queueInLoop(const functor& func) {
	eventque_->addFunctor(func);
}

void eventloop::runAt(const functor& cb, int64_t time) {
	timerque_->addTimer(cb, time);
}

void eventloop::runAfter(const functor &cb, double seconds) {
	int64_t time = static_cast<int64_t>(seconds * 1000000);
	time += timer::getMicroUnixTime();
	timerque_->addTimer(cb, time);
}

const timer* eventloop::runEvery(const functor &cb, double seconds) {
	return timerque_->addTimer(cb, timer::getMicroUnixTime(), seconds);
}

void eventloop::cancelTimer(timer* timer1) {
	if (timer1)
		timerque_->cancelTimer(timer1);
}

void eventloop::doFunctors() {
	functors_.clear();
	eventque_->getFunctors(functors_);
	for (auto& func : functors_)
		func();
}

