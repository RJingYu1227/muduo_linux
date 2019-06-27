#include"elthreadpool.h"
#include"eventloop.h"
#include"tcpserver.h"
#include"logging.h"

#include<pthread.h>


elthreadpool::elthreadpool(int num)
	:start_(0),
	num_(num),
	index_(0) {

}

void elthreadpool::start() {
	if (start_)
		return;

	loops_.clear();
	for (int i = 1; i <= num_; ++i) {
		eventloop* loop_ = new eventloop();
		loops_.push_back(loop_);
	}

	int ret;
	pthread_t temp;
	for (eventloop* ioloop : loops_) {
		ret = pthread_create(&temp, NULL, threadFunc, ioloop);
		if (ret) {
			LOG << "eventloop线程创建失败";
			exit(1);
		}
		//tids_.push_back(temp);
		pthread_detach(temp);
	}
	
	start_ = 1;
}

void elthreadpool::stop() {
	if (!start_)
		return;

	for (eventloop* loop_ : loops_)
		loop_->quit();

	for (eventloop* loop_ : loops_) {
		while (loop_->isLooping());

		loop_->updateThread();
		delete loop_;
	}

	start_ = 0;
}

eventloop* elthreadpool::getLoop() {
	if (num_ == 0)
		return nullptr;
	++index_;
	index_ %= num_;
	return loops_[index_];
}

void* elthreadpool::threadFunc(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();

	return (void*)0;
}
