﻿#pragma once

#include"channel.h"
#include"epoller.h"
#include"timerqueue.h"
#include"eventqueue.h"
#include<pthread.h>
#include<vector>
#include<functional>
#include<sys/eventfd.h>

class channel;
class epoller;
class timer;
class timerqueue;
class eventqueue;

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

	void runInLoop(const functor& func);
	void queueInLoop(const functor& func);//const左值引用

	void runAt(const functor& cb, int64_t time);
	void runAfter(const functor& cb, double seconds);
	timer* runEvery(const functor& cb, double seconds);
	void cancelTimer(timer* timer1);//一个timer只能调用一次

	static eventloop* getEventLoop();

private:
	typedef std::vector<channel*> channel_list;

	void doFunctors();

	pthread_t thread_id_;//第一个初始化
	bool quit_;
	bool looping_;
	int epoll_timeout_;
	epoller* epoller_;
	eventqueue* eventque_;//最后执行
	timerqueue* timerque_;//尽快执行
	channel_list active_channels_;

};

