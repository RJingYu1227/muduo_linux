﻿#pragma once

#include<pax/base/sharedatomic.h>
#include<pax/base/thread.h>

#include<pax/coroutine/coroutine.h>
#include<pax/coroutine/coevent.h>
#include<pax/coroutine/timewheel.h>

#include<queue>

namespace pax {

/*
暂时不适合使用边沿触发模式，请参考coservice::run
使用ET模式时，如果其它线程执行epoll_wait得到对应fd的返回事件
但是，此时对应协程handing_ = 1且此次执行过程中并没有处理该事件
会使该事件很难得到处理，可以参考boost::asio的 事件任务队列 进行更改
*/

class coservice_item;

class coservice :uncopyable {
	friend class coservice_item;
public:

	coservice();
	~coservice();

	size_t run();

private:

	void add(coservice_item* cst);
	void modify(coservice_item* cst);
	void remove(coservice_item* cst);
	void doReactor();
	void setTimeout(uint64_t ms, timenode<coservice_item*>* timenode);

	std::atomic_int32_t item_count_;

	int epfd_;
	std::vector<epoll_event> revents_;
	std::vector<timenode<coservice_item*>*> timenodes_;

	mutex task_mutex_;
	cond task_cond_;
	std::queue<coservice_item*> task_queue_;

	mutex done_mutex_;
	std::vector<coservice_item*> done_items_;

	mutex time_mutex_;
	timewheel<coservice_item*> timewheel_;
};

class coservice_item :
	coroutine_item,
	public coevent {
	friend class coservice;
public:
	typedef std::function<void()> functor;

	static bool create(const functor& func, int fd, const sockaddr_in& addr, coservice* service);
	static bool create(const functor& func, coservice* service);

	static bool create(functor&& func, int fd, const sockaddr_in& addr, coservice* service);
	static bool create(functor&& func, coservice* service);

	static coservice_item* self();

	void yield(double seconds);
	void yield(int ms)override;
	void updateEvents()override;

protected:

	coservice_item(const functor& func, int fd, const sockaddr_in& addr, coservice* service);
	coservice_item(const functor& func, coservice* service);

	coservice_item(functor&& func, int fd, const sockaddr_in& addr, coservice* service);
	coservice_item(functor&& func, coservice* service);

	virtual ~coservice_item();

private:

	thread_local static coservice_item* running_cst_;

	coservice* service_;
	sharedatomic<bool> handling_;
	timenode<coservice_item*> timenode_;

};

inline bool coservice_item::create(const functor& func, int fd, const sockaddr_in& addr, coservice* service) {
	return new(std::nothrow) coservice_item(func, fd, addr, service);
}

inline bool coservice_item::create(const functor& func, coservice* service) {
	return new(std::nothrow) coservice_item(func, service);
}

inline bool coservice_item::create(functor&& func, int fd, const sockaddr_in& addr, coservice* service) {
	return new(std::nothrow) coservice_item(std::move(func), fd, addr, service);
}

inline bool coservice_item::create(functor&& func, coservice* service) {
	return new(std::nothrow) coservice_item(std::move(func), service);
}

inline coservice_item* coservice_item::self() {
	return running_cst_;
}

inline void coservice_item::updateEvents() {
	service_->modify(this);
}

}//namespace pax