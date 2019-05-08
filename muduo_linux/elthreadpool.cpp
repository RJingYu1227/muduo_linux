#include"elthreadpool.h"
#include<assert.h>
#include<stdio.h>
#include<errno.h>


elthreadpool::elthreadpool(int loops) {
	start_ = 0;
	server_ = 0;
	serverloop_ = new eventloop();//主线程退出时会被delete
	io_ = 0;
	ioloop_ = new eventloop();
	eventloop::createQueue(ioloop_);
	loops_ = loops;
}

elthreadpool::~elthreadpool() {
	delete serverloop_;
	delete ioloop_;
}

void elthreadpool::start() {
	assert(!start_);
	int rjy_;
	rjy_ = pthread_create(&server_, NULL, serverThread, serverloop_);
	if (rjy_)
		perror("线程创建失败");
	rjy_ = pthread_create(&io_, NULL, ioThread, ioloop_);
	if (rjy_)
		perror("线程创建失败");
	start_ = 1;
	pthread_join(server_, nullptr);
	pthread_join(io_, nullptr);
}

void* elthreadpool::serverThread(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();
}

void* elthreadpool::ioThread(void* a) {
	eventloop* b = (eventloop*)a;
	b->updateThread();
	b->loop();
}
