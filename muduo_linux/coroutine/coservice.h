#pragma once

#include"coroutine.h"
#include"coevent.h"
#include"timewheel.h"

#include<atomic>
#include<queue>

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
	void setTimeout(unsigned int ms, klinknode<coservice_item*>* timenode);

	std::atomic_int32_t item_count_;

	int epfd_;
	std::vector<epoll_event> revents_;
	std::vector<klinknode<coservice_item*>*> timenodes_;

	kmutex task_mutex_;
	kcond task_cond_;
	std::queue<coservice_item*> task_queue_;

	kmutex done_mutex_;
	std::vector<coservice_item*> done_items_;

	kmutex time_mutex_;
	timewheel<coservice_item*> timewheel_;
};

class coservice_item :
	coroutine_item,
	public coevent {
	friend class coservice;
public:
	typedef std::function<void()> functor;

	inline static bool create(const functor& func, int fd, sockaddr_in& addr, coservice* service);
	inline static bool create(const functor& func, const char* ip, int port, coservice* service);

	inline static bool create(functor&& func, int fd, sockaddr_in& addr, coservice* service);
	inline static bool create(functor&& func, const char* ip, int port, coservice* service);

	inline static coservice_item* self();

	void yield(int ms);
	void updateEvents();

protected:

	coservice_item(const functor& func, int fd, sockaddr_in& addr, coservice* service);
	coservice_item(const functor& func, const char* ip, int port, coservice* service);

	coservice_item(functor&& func, int fd, sockaddr_in& addr, coservice* service);
	coservice_item(functor&& func, const char* ip, int port, coservice* service);

	virtual ~coservice_item();

private:

	thread_local static coservice_item* running_cst_;

	coservice* service_;
	std::atomic_bool handling_;
	klinknode<coservice_item*> timenode_;

};

bool coservice_item::create(const functor& func, int fd, sockaddr_in& addr, coservice* service) {
	return new(std::nothrow) coservice_item(func, fd, addr, service);
}

bool coservice_item::create(const functor& func, const char* ip, int port, coservice* service) {
	return new(std::nothrow) coservice_item(func, ip, port, service);
}

bool coservice_item::create(functor&& func, int fd, sockaddr_in& addr, coservice* service) {
	return new(std::nothrow) coservice_item(std::move(func), fd, addr, service);
}

bool coservice_item::create(functor&& func, const char* ip, int port, coservice* service) {
	return new(std::nothrow) coservice_item(std::move(func), ip, port, service);
}

coservice_item* coservice_item::self() {
	return running_cst_;
}

//暂时不适合使用边沿触发模式，请参考coservice::run
//使用ET模式时，如果其它线程执行epoll_wait得到对应fd的返回事件
//但是，此时对应协程handing_ = 1且此次执行过程中并没有处理该事件
//会使该事件很难得到处理，可以参考boost::asio的 事件任务队列 进行更改