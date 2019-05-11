#include"eventloop.h"
#include<iostream>
#include<assert.h>
#include<unistd.h>
using namespace::std;

__thread eventloop* loop_inthisthread_ = 0;

eventloop* eventloop::get_eventloop() {
	return loop_inthisthread_;
}

void eventloop::updateThread() {
	thread_id_ = pthread_self();
	loop_inthisthread_ = this;
	cout << "事件循环" << loop_inthisthread_ << "的新线程" << thread_id_ << endl;
}

eventloop::eventloop() :
	epoll_timeout_(-1),
	looping_(0),
	quit_(0),
	thread_id_(pthread_self()),
	epoller_(new epoller(this)) {
	cout << "在主线程" << thread_id_ << "创建事件循环" << this << endl;
}

eventloop::~eventloop() {
	assert(!looping_);
	delete epoller_;
	close(wakeupfd_);
	loop_inthisthread_ = nullptr;
}

void eventloop::assertInLoopThread() {
	if (thread_id_ != pthread_self())
		cout << "事件循环" << this << "属于线程" << thread_id_ << "目前线程为" << pthread_self() << endl;
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

	while (!quit_) {
		active_channels_.clear();
		epoller_->epoll(epoll_timeout_, &active_channels_);
		for (channel* ch : active_channels_)
			ch->handleEvent();
	}

	cout << "事件循环" << this << "停止" << endl;
	looping_ = 0;
}

void eventloop::quit() {
	quit_ = 1;
}

void eventloop::runInLoop(const functor& cb) {
	if (isInLoopThread())
		cb();
	else {
		pthread_mutex_lock(&lock_);

		pending_functors_.push_back(cb);
		eventfd_write(wakeupfd_, 1);//如果使用count_，可能会造成死锁（write被阻塞）。

		pthread_mutex_unlock(&lock_);
	}
}

void eventloop::doFunctors() {
	std::vector<functor> temp_;
	pthread_mutex_lock(&lock_);

	eventfd_read(wakeupfd_, &count_);
	temp_.swap(pending_functors_);

	pthread_mutex_unlock(&lock_);
	for (functor cb : temp_)
		cb();
}

void eventloop::newConn(tcpconnection* &conn, int fd, sockaddr_in* cliaddr) {
	channel* ch;
	ch = m_pool_->newConn(conn);
	new(conn)tcpconnection(this, ch, fd, cliaddr);
}

void eventloop::removeConn(tcpconnection* conn) {
	assertInLoopThread();
	m_pool_->deleteConn(conn);
}

void eventloop::createQueue(eventloop* loop) {
	loop->m_pool_ = new memorypool();
	loop->wakeupfd_ = eventfd(0, 0);
	channel* channel_ = new channel(loop, loop->wakeupfd_);
	channel_->setReadCallback(std::bind(&eventloop::doFunctors, loop));
	channel_->enableReading();
	loop->lock_ = PTHREAD_MUTEX_INITIALIZER;
}
