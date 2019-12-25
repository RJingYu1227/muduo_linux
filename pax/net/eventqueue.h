#pragma once

#include<pax/base/thread.h>
#include<pax/base/uncopyable.h>

#include<pax/net/channel.h>

#include<sys/eventfd.h>

#include<functional>
#include<vector>

namespace pax {

class eventloop;

class eventqueue :uncopyable {
public:
	typedef std::function<void()> functor;

	explicit eventqueue(eventloop* loop);
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