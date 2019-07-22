﻿#include"channel.h"
#include"eventloop.h"

#include<sys/epoll.h>
#include<assert.h>

const int channel::kNoneEvent = 0;
const int channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int channel::kWriteEvent = EPOLLOUT;

channel::channel(eventloop* loop, int fd)
	:loop_(loop),
	fd_(fd),
	event_(0),
	revent_(0),
	mark_(-1) {

}

channel::~channel() {
	assert(mark_ == -1);
}

void channel::update() {
	loop_->updateChannel(this);
}

void channel::remove() {
	loop_->removeChannel(this);
}

void channel::handleEvent() {
	if (revent_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
		if (readCallback)
			readCallback();
	if (revent_ & EPOLLOUT)
		if (writeCallback)
			writeCallback();
	if (revent_ & EPOLLERR)
		if (errorCallback)
			errorCallback();
	if (revent_ & EPOLLHUP)
		if (closeCallback)
			closeCallback();
}