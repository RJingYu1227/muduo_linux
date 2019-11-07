#pragma once

#include"blockqueue.h"
#include"coroutine.h"
#include"coevent.h"

class coservice :uncopyable {
public:
	typedef std::function<void()> functor;

	coservice();
	~coservice();

	size_t run();

	class coservice_item :
		public coroutine::coroutine_item,
		public coevent {
		friend class coservice;
	public:

		coservice_item(int fd, const functor& func, coservice* service);
		coservice_item(int fd, functor&& func, coservice* service);
		~coservice_item();

		inline void updateEvents();

	private:

		static void coroutineFunc(coservice_item* cst);

		coservice* service_;
		functor Func;

	};

private:

	void add(coservice_item* cst);
	void modify(coservice_item* cst);
	void remove(coservice_item* cst);

	int epfd_;
	std::vector<epoll_event> revents_;

	blockqueue<coservice_item*> queue_;

};

void coservice::coservice_item::updateEvents() {
	service_->modify(this);
}