﻿#include<pax/net/elthreadpool.h>
#include<pax/net/eventloop.h>

using namespace pax;

elthreadpool::elthreadpool(int num)
	:started_(0),
	loop_num_(num),
	index_(0),
	current_num_(0) {

}

void elthreadpool::start() {
	if (started_)
		return;

	for (int i = 1; i <= loop_num_; ++i) {
		thread thread_(std::bind(&elthreadpool::threadFunc, this));
		thread_.start();
	}
	
	while (current_num_ != loop_num_);

	started_ = 1;
}

void elthreadpool::stop() {
	if (!started_)
		return;

	for (eventloop* loop_ : loops_)
		loop_->quit();

	while (current_num_ != 0);

	loops_.clear();
	started_ = 0;
}

eventloop* elthreadpool::getLoop() {
	if (loop_num_ == 0)
		return nullptr;

	++index_;
	index_ %= loop_num_;
	return loops_[index_];
}

void elthreadpool::threadFunc() {
	eventloop loop_;
	{
		lock<mutex> x(&lock_);
		loops_.push_back(&loop_);
	}
	++current_num_;

	loop_.loop();

	--current_num_;
}