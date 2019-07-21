#include"elthreadpool.h"
#include"eventloop.h"
#include"kthread.h"

elthreadpool::elthreadpool(int num)
	:started_(0),
	loop_num_(num),
	index_(0) {

}

void elthreadpool::start() {
	if (started_)
		return;

	for (int i = 1; i <= loop_num_; ++i) {
		kthread thread_(std::bind(&elthreadpool::threadFunc, this));
		thread_.start();
	}
	
	while (static_cast<int>(loops_.size()) != loop_num_);

	started_ = 1;
}

void elthreadpool::stop() {
	if (!started_)
		return;

	for (eventloop* loop_ : loops_)
		loop_->quit();

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
	static kmutex mutex_;

	eventloop loop_;
	{
		klock<kmutex> x(&mutex_);
		loops_.push_back(&loop_);
	}
	loop_.loop();
}
