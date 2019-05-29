﻿#include"eventloop.h"
#include"logging.h"
#include<iostream>
#include<assert.h>
#include<unistd.h>

__thread eventloop* loop_inthisthread_ = nullptr;

eventloop* eventloop::getEventLoop() {
	return loop_inthisthread_;
}

void eventloop::updateThread() {
	thread_id_ = pthread_self();
	loop_inthisthread_ = this;
	LOG << "事件循环：" << loop_inthisthread_ << " 的新线程：" << thread_id_;
}

eventloop::eventloop() :
	thread_id_(pthread_self()),
	quit_(0),
	looping_(0),
	epoll_timeout_(-1),
	epoller_(new epoller(this)),
	eventque_(new eventqueue(this)),
	timerque_(new timerqueue(this)) {

	loop_inthisthread_ = this;
	LOG << "在线程：" << thread_id_ << " 创建事件循环：" << this;
}

eventloop::~eventloop() {
	assert(!looping_);
	delete timerque_;
	delete eventque_;
	delete epoller_;
	loop_inthisthread_ = nullptr;

	LOG << "在线程：" << pthread_self() << " 关闭事件循环：" << this;
}

void eventloop::assertInLoopThread() {
	if (thread_id_ != pthread_self())
		LOG << "事件循环：" << this << " 属于线程：" << thread_id_ << " 目前线程为：" << pthread_self();
}

bool eventloop::isInLoopThread()const {
	return thread_id_ == pthread_self();
}

void eventloop::updateChannel(channel* ch) {
	assert(ch->ownerLoop() == this);
	assertInLoopThread();
	epoller_->updateChannel(ch);
}

void eventloop::removeChannel(channel* ch) {
	assert(ch->ownerLoop() == this);
	assertInLoopThread();
	epoller_->removeChannel(ch);
}

void eventloop::loop() {
	assert(!looping_);
	assertInLoopThread();
	looping_ = 1;
	quit_ = 0;
	LOG << "事件循环：" << this << " 开始";

	while (!quit_) {
		active_channels_.clear();
		epoller_->doEpoll(epoll_timeout_, &active_channels_);
		for (channel* ch : active_channels_)
			ch->handleEvent();
		doFunctors();//注意这里的执行顺序
	}
	doFunctors();//注意这里

	LOG << "事件循环：" << this << " 停止";
	looping_ = 0;
}

void eventloop::quit() {
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

timer* eventloop::runEvery(const functor &cb, double seconds) {
	return timerque_->addTimer(cb, timer::getMicroUnixTime(), seconds);
}

void eventloop::cancelTimer(timer* timer1) {
	if (timer1)
		timerque_->cancelTimer(timer1);
}

void eventloop::doFunctors() {
	std::vector<functor> functors_;
	eventque_->getFunctors(functors_);
	for (functor& func : functors_)
		func();
}

