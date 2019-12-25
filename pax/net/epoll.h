#pragma once

#include<pax/net/poller.h>

#include<sys/epoll.h>

namespace pax {

//默认水平触发，listenfd改为边沿触发
class epoll :public poller {
public:

	explicit epoll(eventloop* loop);
	~epoll();

	void doPoll(int timeoutms, channellist& active_channels_);
	void updateChannel(channel* ch)override;
	void removeChannel(channel* ch)override;

private:
	typedef std::vector<epoll_event> event_list;

	static const int kInitEventListSize = 128;

	void fillActiveChannels(int numevents, channellist& active_channels_);

	int epollfd_;
	event_list events_;

};

}//namespace pax