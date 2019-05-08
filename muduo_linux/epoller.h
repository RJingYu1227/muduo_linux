#ifndef EPOLLER_H
#define EPOLLER_H

#include"channel.h"
#include"eventloop.h"
#include<vector>
#include<sys/epoll.h>
#include<map>

class channel;
class eventloop;

class epoller {
public:
	typedef std::vector<channel*> channellist;

	epoller(eventloop* loop);
	~epoller();

	void epoll(int timeoutms, channellist* active_channels_);
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	void assertInLoopThread();

private:
	void fillActiveChannels(int numevents, channellist* active_channels_)const;

	typedef std::map<int, channel*> channel_map;
	
	eventloop* loop_;
	int epollfd_;
	epoll_event events_[100];
	channel_map channels_;
};

#endif // !MUDUO_NET_EPOLLER_H

