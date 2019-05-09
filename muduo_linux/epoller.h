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
	typedef std::map<int, channel*> channel_map;
	typedef std::vector<epoll_event> event_list;

	static const int kInitEventListSize = 16;

	void fillActiveChannels(int numevents, channellist* active_channels_)const;
	
	eventloop* loop_;
	int epollfd_;
	event_list events_;
	channel_map channels_;
};

#endif // !MUDUO_NET_EPOLLER_H

