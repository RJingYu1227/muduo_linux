#pragma once

#include"blockqueue.h"
#include"coroutine.h"
#include"coevent.h"
#include"timewheel.h"

#include<atomic>

class coservice_item;

class coservice :uncopyable {
	friend class coservice_item;
public:

	coservice();
	~coservice();//请确保析构函数在最后调用

	size_t run();

private:

	void add(coservice_item* cst);
	void modify(coservice_item* cst);
	void remove(coservice_item* cst);
	void getRevents();
	void getTimeout();

	std::atomic_int32_t item_count_;

	int epfd_;
	std::vector<epoll_event> revents_;

	blockqueue<coservice_item*> cst_queue_;

	kmutex done_items_mutex_;
	std::vector<coservice_item*> done_items_;

	kmutex time_mutex_;
	std::vector<klinknode<coservice_item*>*> timenodes_;
	timewheel<coservice_item*> timewheel_;
};

class coservice_item :
	coroutine::coroutine_item,
	public coevent {
	friend class coservice;
public:
	typedef std::function<void()> functor;

	inline static coservice_item* create(int fd, const functor& func, coservice* service);
	inline static coservice_item* create(int fd, functor&& func, coservice* service);
	inline static coservice_item* self();

	inline void updateEvents();
	void setTimeout(unsigned int ms);

protected:

	coservice_item(int fd, const functor& func, coservice* service);
	coservice_item(int fd, functor&& func, coservice* service);
	~coservice_item();

private:

	thread_local static coservice_item* running_cst_;

	coservice* service_;
	std::atomic_bool handling_;
	klinknode<coservice_item*> timenode_;

};

coservice_item* coservice_item::create(int fd, const functor& func, coservice* service) {
	return new coservice_item(fd, func, service);
}

coservice_item* coservice_item::create(int fd, functor&& func, coservice* service) {
	return new coservice_item(fd, std::move(func), service);
}

coservice_item* coservice_item::self() {
	return running_cst_;
}

void coservice_item::updateEvents() {
	service_->modify(this);
}

//暂时不适合使用边沿触发模式，请参考coservice::run
//使用ET模式时，如果其它线程执行epoll_wait得到对应fd的返回事件
//但是，此时对应协程handing_ = 1且此次执行过程中并没有处理该事件
//会使该事件很难得到处理，可以参考boost::asio的 事件任务队列 进行更改