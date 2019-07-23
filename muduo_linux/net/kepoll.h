#pragma once

#include"poller.h"

#include<sys/epoll.h>

//默认水平触发，listenfd改为边沿触发
class kepoll :public poller {
public:

	kepoll(eventloop* loop);
	~kepoll();

	void doPoll(int timeoutms, channellist& active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);

private:
	typedef std::vector<epoll_event> event_list;

	static const int kInitEventListSize = 128;

	void fillActiveChannels(int numevents, channellist& active_channels_);

	int epollfd_;
	event_list events_;

};
