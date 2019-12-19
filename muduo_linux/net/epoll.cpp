#include"epoll.h"
#include"channel.h"

#include"log/logging.h"

#include<assert.h>
#include<unistd.h>

using namespace pax;

epoll::epoll(eventloop* loop)
	:poller(loop),
	epollfd_(epoll_create1(EPOLL_CLOEXEC)),
	events_(kInitEventListSize) {

	assert(epollfd_ > 0);
}

epoll::~epoll() {
	close(epollfd_);
}

void epoll::doPoll(int timeoutms, channellist& active_channels_) {
	int numevents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutms);
	//调用epoll_wait的时候,将readylist中的epitem出列,将触发的事件拷贝到用户空间
	if (numevents > 0) {
		fillActiveChannels(numevents, active_channels_);
		if (static_cast<size_t>(numevents) == events_.size())
			events_.resize(events_.size() * 2);
	}
	else if (numevents == -1)
		LOG << "epoll调用出错，errno = " << errno;

}

void epoll::updateChannel(channel* ch) {
	int fd = ch->getFd();
	epoll_event ev;
	ev.events = ch->getEvent();
	ev.data.ptr = ch;

	if (ch->getMark() == -1) {
		assert(channels_.find(fd) == channels_.end());
		epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
		channels_.emplace(fd, ch);
		ch->setMark(1);
	}
	else {
		auto iter = channels_.find(fd);
		assert(iter != channels_.end() && iter->second == ch);
		epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
	}
}

void epoll::removeChannel(channel* ch) {
	int fd = ch->getFd();

	if (ch->getMark() == -1) {
		assert(channels_.find(fd) == channels_.end());
		return;
	}

	epoll_event ev;
	ev.events = ch->getEvent();
	ev.data.ptr = ch;

	auto iter = channels_.find(fd);
	assert(iter != channels_.end() && iter->second == ch);
	channels_.erase(fd);
	epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);
	ch->setMark(-1);

}

void epoll::fillActiveChannels(int numevents_, channellist& active_channels_) {
	for (int i = 0; i < numevents_; ++i) {
		channel* ch = (channel*)(events_[i].data.ptr);
		ch->setRevent(events_[i].events);
		active_channels_.push_back(ch);
	}
}