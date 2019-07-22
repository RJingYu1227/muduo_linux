﻿#include"kepoll.h"
#include"logging.h"
#include"channel.h"

#include<assert.h>
#include<unistd.h>
#include<strings.h>

kepoll::kepoll(eventloop* loop)
	:poller(loop),
	epollfd_(epoll_create1(EPOLL_CLOEXEC)),
	events_(kInitEventListSize) {

	assert(epollfd_ > 0);
}

kepoll::~kepoll() {
	close(epollfd_);
}

void kepoll::doPoll(int timeoutms, channellist& active_channels_) {
	int numevents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutms);
	//调用epoll_wait的时候,将readylist中的epitem出列,将触发的事件拷贝到用户空间
	if (numevents > 0) {
		fillActiveChannels(numevents, active_channels_);
		if (static_cast<size_t>(numevents) == events_.size())
			events_.resize(events_.size() * 2);
	}
	else
		LOG << "epoll调用出错，errno = " << errno;

}

void kepoll::updateChannel(channel* ch) {
	int fd_ = ch->getFd();
	epoll_event ev;
	bzero(&ev, sizeof ev);
	ev.events = ch->getEvent();
	ev.data.fd = fd_;
	ev.data.ptr = ch;

	if (ch->getMark() == -1) {
		assert(channels_.find(fd_) == channels_.end());
		epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd_, &ev);
		channels_.emplace(fd_, ch);
		ch->setMark(1);
	}
	else {
		auto iter = channels_.find(fd_);
		assert(iter != channels_.end());
		assert(iter->second = ch);
		epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd_, &ev);
	}
}

void kepoll::removeChannel(channel* ch) {
	int fd_ = ch->getFd();

	if (ch->getMark() == -1) {
		assert(channels_.find(fd_) == channels_.end());
		return;
	}

	epoll_event ev;
	bzero(&ev, sizeof ev);
	ev.events = ch->getEvent();
	ev.data.fd = fd_;
	ev.data.ptr = ch;

	auto iter = channels_.find(fd_);
	assert(iter != channels_.end());
	assert(iter->second = ch);
	channels_.erase(fd_);
	epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd_, &ev);
	ch->setMark(-1);

}

void kepoll::fillActiveChannels(int numevents_, channellist& active_channels_) {
	assert(static_cast<size_t>(numevents_) <= events_.size());
	for (int i = 0; i < numevents_; ++i) {
		channel* ch = (channel*)(events_[i].data.ptr);
		ch->setRevent(events_[i].events);
		active_channels_.push_back(ch);
	}
}