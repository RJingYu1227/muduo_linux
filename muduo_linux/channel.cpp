#include"channel.h"
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
	if (revent_ & EPOLLHUP)
		if (close_callback_)
			close_callback_();
	if (revent_ & EPOLLERR)
		if (error_callback_)
			error_callback_();
	if (revent_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
		if (read_callback_)
			read_callback_();
	if (revent_ & EPOLLOUT)
		if (write_callback_)
			write_callback_();
}