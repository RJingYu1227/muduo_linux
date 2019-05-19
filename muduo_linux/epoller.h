#pragma once

#include"channel.h"
#include"eventloop.h"
#include<vector>
#include<sys/epoll.h>
#include<unordered_map>

class channel;
class eventloop;

class epoller {
public:
	typedef std::vector<channel*> channellist;

	epoller(eventloop* loop);
	~epoller();

	void doEpoll(int timeoutms, channellist* active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	void assertInLoopThread();

private:
	typedef std::unordered_map<int, channel*> channel_map;
	typedef std::vector<epoll_event> event_list;

	static const int kInitEventListSize = 16;

	void fillActiveChannels(int numevents, channellist* active_channels_)const;
	
	eventloop* loop_;
	int epollfd_;
	event_list events_;
	channel_map channels_;
};
