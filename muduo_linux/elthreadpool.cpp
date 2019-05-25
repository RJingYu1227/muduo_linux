#include"elthreadpool.h"
#include"logging.h"
#include<assert.h>
#include<pthread.h>


elthreadpool::elthreadpool(int num)
	:start_(0),
	num_(num - 1),
	index_(0) {
	serverloop_ = new eventloop();
	for (int i = 1; i <= num_; ++i) {
		eventloop* ioloop_ = new eventloop();
		ioloops_.push_back(ioloop_);
	}
}

elthreadpool::~elthreadpool() {
	delete serverloop_;
	for (eventloop* ioloop : ioloops_)
		delete ioloop;
}

void elthreadpool::start() {
	assert(!start_);
	int ret;
	pthread_t temp;
	for (eventloop* ioloop : ioloops_) {
		ret = pthread_create(&temp, NULL, ioThread, ioloop);
		if (ret)
			LOG << "线程创建失败";
		//pthread_detach(temp);
	}
	
	start_ = 1;
	serverloop_->loop();
}

eventloop* elthreadpool::getIoLoop() {
	if (num_ == 0)
		return serverloop_;
	++index_;
	index_ %= num_;
	return ioloops_[index_];
}

void* elthreadpool::ioThread(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();

	return (void*)0;
}
