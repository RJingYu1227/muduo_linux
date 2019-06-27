#pragma once

#include"poller.h"

#include<vector>
#include<sys/epoll.h>
#include<unordered_map>

class channel;
class eventloop;

class epoller :public poller {
public:
	typedef std::vector<channel*> channellist;

	epoller(eventloop* loop);
	~epoller();

	void doPoll(int timeoutms, channellist& active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	void assertInLoopThread();

private:
	typedef std::unordered_map<int, channel*> channel_map;
	typedef std::vector<epoll_event> event_list;

	static const int kInitEventListSize = 1024;

	void fillActiveChannels(int numevents, channellist& active_channels_)const;

	int epollfd_;
	event_list events_;

};
