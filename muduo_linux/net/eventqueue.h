#pragma once

#include"channel.h"

#include"base/thread.h"
#include"base/uncopyable.h"

#include<sys/eventfd.h>

#include<functional>
#include<vector>

namespace pax {

class eventloop;

class eventqueue :uncopyable {
public:
	typedef std::function<void()> functor;

	eventqueue(eventloop* loop);
	~eventqueue();

	void wakeup();
	void addFunctor(const functor& func);
	void addFunctor(functor&& func);
	void getFunctors(std::vector<functor>& vec);

private:
	void handleRead();

	int fd_;
	eventfd_t count_;
	mutex lock_;
	eventloop* loop_;
	channel channel_;
	std::vector<functor> functors_;

};

}//namespace pax