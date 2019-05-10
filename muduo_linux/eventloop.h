#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include<pthread.h>
#include<vector>
#include<memory>
#include<sys/eventfd.h>
#include<functional>
#include"channel.h"
#include"epoller.h"
#include"tcpconnection.h"

class channel;
class epoller;
class tcpconnection;

//typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
typedef std::function<void()> functor;

class eventloop {
public:
	eventloop();
	~eventloop();

	void updateThread();
	void loop();
	void quit();
	void assertInLoopThread();
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	bool isInLoopThread()const;
	void runInLoop(const functor& cb);

	static void createQueue(eventloop* loop);
	static eventloop* get_eventloop();

private:
	typedef std::vector<channel*> channel_list;
	//typedef std::vector<tcpconn_ptr> tcpconn_queue;

	//消息队列
	void doFunctors();
	pthread_mutex_t lock_;
	eventfd_t count_;
	std::vector<functor> pending_functors_;
	int wakeupfd_;
	timeval select_timeout_;

	int epoll_timeout_;
	bool quit_;
	channel_list active_channels_;
	epoller* epoller_;
	bool looping_;
	pthread_t thread_id_;	

};

#endif // !MUDUO_NET_CHANNEL_H

