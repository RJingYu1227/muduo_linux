#pragma once

#include"eventloop.h"
#include"channel.h"
#include"uncopyable.h"

#include<functional>
#include<vector>
#include<sys/eventfd.h>
#include<pthread.h>

class eventloop;
class channel;

class eventqueue :uncopyable {
public:
	typedef std::function<void()> functor;

	eventqueue(eventloop* loop);
	~eventqueue();

	void wakeup();
	void addFunctor(const functor& func);
	void getFunctors(std::vector<functor>& vec);

private:
	void handleRead();

	int fd_;
	eventfd_t count_;
	pthread_mutex_t lock_;
	eventloop* loop_;
	channel* channel_;
	std::vector<functor> functors_;

};

