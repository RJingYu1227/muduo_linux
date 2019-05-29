#include"elthreadpool.h"
#include"logging.h"
#include<assert.h>
#include<pthread.h>


elthreadpool::elthreadpool(eventloop* baseloop, int num)
	:start_(0),
	num_(num),
	index_(0),
	baseloop_(baseloop) {
	for (int i = 1; i <= num; ++i) {
		eventloop* loop_ = new eventloop();
		loops_.push_back(loop_);
	}
}

elthreadpool::~elthreadpool() {
	start_ = 0;

	for (eventloop* loop_ : loops_)
		loop_->quit();

	for (eventloop* loop_ : loops_) {
		while (loop_->isLooping());

		loop_->updateThread();
		delete loop_;
	}
}

void elthreadpool::start() {
	assert(!start_);
	int ret;
	pthread_t temp;
	for (eventloop* ioloop : loops_) {
		ret = pthread_create(&temp, NULL, loopThreadFunc, ioloop);
		if (ret) {
			LOG << "eventloop线程创建失败";
			exit(1);
		}
		//tids_.push_back(temp);
		pthread_detach(temp);
	}
	
	start_ = 1;
}

eventloop* elthreadpool::getLoop() {
	if (num_ == 0)
		return baseloop_;
	++index_;
	index_ %= num_;
	return loops_[index_];
}

void* elthreadpool::loopThreadFunc(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();

	return (void*)0;
}
