#pragma once

#include<pax/coroutine/coroutine.h>
#include<pax/coroutine/coevent.h>
#include<pax/coroutine/timewheel.h>

#include<vector>

namespace pax {

//one loop per thread
class coloop_item;

class coloop :uncopyable {
	friend class coloop_item;
public:

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

	mutex time_mutex_;
	std::vector<klinknode<coloop_item*>*> timenodes_;
	timewheel<coloop_item*> timewheel_;

};

class coloop_item :
	coroutine_item,
	public coevent {
	friend class coloop;
public:
	typedef std::function<void()> functor;

	inline static bool create(const functor& func, int fd, sockaddr_in& addr, coloop* loop);
	inline static bool create(const functor& func, const char* ip, int port, coloop* loop);

	inline static bool create(functor&& func, int fd, sockaddr_in& addr, coloop* loop);
	inline static bool create(functor&& func, const char* ip, int port, coloop* loop);

	inline static coloop_item* self();

	void yield(int ms);
	void updateEvents();

protected:

	coloop_item(const functor& func, int fd, sockaddr_in& addr, coloop* loop);
	coloop_item(const functor& func, const char* ip, int port, coloop* loop);

	coloop_item(functor&& func, int fd, sockaddr_in& addr, coloop* loop);
	coloop_item(functor&& func, const char* ip, int port, coloop* loop);

	~coloop_item();

private:

	thread_local static coloop_item* running_cpt_;

	coloop* loop_;
	klinknode<coloop_item*> timenode_;

};

bool coloop_item::create(const functor& func, int fd, sockaddr_in& addr, coloop* loop) {
	return new(std::nothrow) coloop_item(func, fd, addr, loop);
}

bool coloop_item::create(const functor& func, const char* ip, int port, coloop* loop) {
	return new(std::nothrow) coloop_item(func, ip, port, loop);
}

bool coloop_item::create(functor&& func, int fd, sockaddr_in& addr, coloop* loop) {
	return new(std::nothrow) coloop_item(std::move(func), fd, addr, loop);
}

bool coloop_item::create(functor&& func, const char* ip, int port, coloop* loop) {
	return new(std::nothrow) coloop_item(std::move(func), ip, port, loop);
}

coloop_item* coloop_item::self() {
	return running_cpt_;
}

}//namespace pax