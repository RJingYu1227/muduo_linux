#include"elthreadpool.h"
#include<assert.h>
#include<stdio.h>
#include<errno.h>


elthreadpool::elthreadpool(int loops)
	:start_(0),
	loop_num_(loops - 1),
	loop_index_(0) {
	serverloop_ = new eventloop();//主线程退出时会被delete
	for (int i = 1; i <= loop_num_; ++i) {
		eventloop* ioloop_ = new eventloop();
		eventloop::createQueue(ioloop_);
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
			perror("线程创建失败");
	}
	
	start_ = 1;
	serverloop_->loop();
}

eventloop* elthreadpool::getIoLoop() {
	++loop_index_;
	loop_index_ %= loop_num_;
	return ioloops_[loop_index_];
}

void* elthreadpool::ioThread(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();

	return (void*)0;
}
