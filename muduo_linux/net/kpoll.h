#pragma once

#include"poller.h"

#include<sys/poll.h>

class kpoll :public poller {
public:

	kpoll(eventloop* loop)
		:poller(loop) {

	}
	~kpoll() = default;

	void doPoll(int timeoutms, channellist& active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);

private:
	typedef std::vector<struct pollfd> pollfd_list;

	void fillActiveChannels(int numevents, channellist& active_channels_);

	pollfd_list pollfds_;

};
