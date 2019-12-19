#pragma once

#include<pax/net/poller.h>

#include<sys/poll.h>

namespace pax {

class poll :public poller {
public:

	poll(eventloop* loop)
		:poller(loop) {

	}
	~poll() = default;

	void doPoll(int timeoutms, channellist& active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);

private:
	typedef std::vector<struct pollfd> pollfd_list;

	void fillActiveChannels(int numevents, channellist& active_channels_);

	pollfd_list pollfds_;

};

}//namespace pax