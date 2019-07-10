#pragma once

#include"channel.h"
#include"kthread.h"
#include"uncopyable.h"

#include<functional>
#include<vector>
#include<sys/eventfd.h>

class eventloop;

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
	kmutex lock_;
	eventloop* loop_;
	channel channel_;
	std::vector<functor> functors_;

};

