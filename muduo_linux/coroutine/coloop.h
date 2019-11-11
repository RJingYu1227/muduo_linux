#pragma once

#include"coroutine.h"
#include"coevent.h"
#include"timewheel.h"

#include<vector>

//one loop per thread
class coloop_item;

class coloop :uncopyable {
	friend class coloop_item;
public:

	static void yield(unsigned int ms);

	coloop();
	~coloop();

	void loop();
	void quit() { quit_ = 1; }

private:

	void add(coloop_item* cpt);
	void modify(coloop_item* cpt);
	void remove(coloop_item* cpt);
	void doItem(coloop_item* cpt);
	void setTimeout(unsigned int ms, klinknode<coloop_item*>* timenode);

	bool looping_;
	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	kmutex time_mutex_;
	std::vector<klinknode<coloop_item*>*> timenodes_;
	timewheel<coloop_item*> timewheel_;

};

class coloop_item :
	coroutine::coroutine_item,
	public coevent {
	friend class coloop;
public:
	typedef std::function<void()> functor;

	inline static bool create(int fd, const functor& func, coloop* loop);
	inline static bool create(int fd, functor&& func, coloop* loop);
	inline static coloop_item* self();

	inline void updateEvents();

protected:

	coloop_item(int fd, const functor& func, coloop* loop);
	coloop_item(int fd, functor&& func, coloop* loop);
	~coloop_item();

private:

	thread_local static coloop_item* running_cpt_;

	bool started_;
	coloop* loop_;
	klinknode<coloop_item*> timenode_;

};

bool coloop_item::create(int fd, const functor& func, coloop* loop) {
	return new(std::nothrow) coloop_item(fd, func, loop);
}

bool coloop_item::create(int fd, functor&& func, coloop* loop) {
	return new(std::nothrow) coloop_item(fd, std::move(func), loop);
}

coloop_item* coloop_item::self() {
	return running_cpt_;
}

void coloop_item::updateEvents() {
	loop_->modify(this);
}