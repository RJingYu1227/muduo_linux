#pragma once

#include"channel.h"
#include"epoller.h"
#include"tcpconnection.h"
#include<netinet/in.h>
#include<pthread.h>
#include<vector>
#include<memory>
#include<sys/eventfd.h>
#include<functional>

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
	void queueInLoop(const functor& cb);//const左值引用

	static eventloop* get_eventloop();

private:
	typedef std::vector<channel*> channel_list;
	//typedef std::vector<tcpconn_ptr> tcpconn_queue;

	//消息队列
	void handleRead();
	void doFunctors();
	pthread_mutex_t lock_;
	eventfd_t count_;
	std::vector<functor> pending_functors_;
	int eventfd_;

	bool quit_ = 0;
	bool looping_ = 0;
	int epoll_timeout_;
	epoller* epoller_;
	pthread_t thread_id_;	
	channel_list active_channels_;

};

