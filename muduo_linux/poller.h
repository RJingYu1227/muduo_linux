#pragma once

#include"uncopyable.h"
#include"eventloop.h"

#include<unordered_map>
#include<vector>

class channel;
class eventloop;

class poller :uncopyable {
public:
	typedef std::vector<channel*> channellist;

	poller(eventloop* loop)
		:loop_(loop) {};
	virtual ~poller() = default;

	virtual void doPoll(int, channellist&) = 0;
	virtual void updateChannel(channel* ch) = 0;
	virtual void removeChannel(channel* ch) = 0;
	void assertInLoopThread();

protected:
	typedef std::unordered_map<int, channel*> channel_map;

	eventloop* loop_;
	channel_map channels_;

};

