#pragma once

#include"channel.h"
#include"epoller.h"
#include"timerqueue.h"
#include<pthread.h>
#include<vector>
#include<functional>
#include<sys/eventfd.h>

class channel;
class epoller;
class timer;
class timerqueue;

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

	void runAt(const functor& cb, int64_t time);
	void runAfter(const functor& cb, double seconds);
	timer* runEvery(const functor& cb, double seconds);
	void cancelTimer(timer* timer1);//一个timer只能调用一次

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
	timerqueue* timer_q_;
	pthread_t thread_id_;	
	channel_list active_channels_;

};

