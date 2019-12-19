#include"poll.h"
#include"channel.h"

#include"log/logging.h"

#include<assert.h>

using namespace pax;

void poll::doPoll(int timeoutms, channellist& active_channels_) {
	int numevents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutms);
	if (numevents > 0)
		fillActiveChannels(numevents, active_channels_);
	else if (numevents == -1)
		LOG << "poll调用出错，errno = " << errno;
}

void poll::updateChannel(channel* ch) {
	int fd = ch->getFd();

	if (ch->getMark() == -1) {
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = static_cast<short>(ch->getEvent());
		assert(channels_.find(fd) == channels_.end());

		pollfds_.push_back(pfd);
		channels_.emplace(fd, ch);
		ch->setMark(static_cast<int>(pollfds_.size() - 1));
	}
	else {
		auto iter = channels_.find(fd);
		assert(iter != channels_.end() && iter->second == ch);
		int idx = ch->getMark();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
		pollfd& pfd = pollfds_[idx];
		assert(pfd.fd == fd || pfd.fd == -fd - 1);

		pfd.fd = fd;
		pfd.events = static_cast<short>(ch->getEvent());
		if (ch->isNoneEvent())
			pollfds_[idx].fd = -fd - 1;
	}
}

void poll::removeChannel(channel* ch) {
	int fd = ch->getFd();
	int idx = ch->getMark();

	if (idx == -1) {
		assert(channels_.find(fd) == channels_.end());
		return;
	}

	auto iter = channels_.find(fd);
	assert(iter != channels_.end() && iter->second == ch);
	channels_.erase(fd);
	assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
	if (static_cast<size_t>(idx) == pollfds_.size() - 1)
		pollfds_.pop_back();
	else {
		pollfds_[idx] = pollfds_.back();
		pollfds_.pop_back();
		int efd = pollfds_[idx].fd;
		if (efd < 0)
			efd = -efd - 1;
		channels_[efd]->setMark(idx);
	}
	ch->setMark(-1);
}

void poll::fillActiveChannels(int numevents_, channellist& active_channels_) {
	for (auto iter = pollfds_.begin(); iter != pollfds_.end() && numevents_; ++iter) {
		if (iter->revents > 0) {
			--numevents_;
			channel* ch = channels_[iter->fd];
			ch->setRevent(iter->revents);
			active_channels_.push_back(ch);
		}
	}
}