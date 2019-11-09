#pragma once

#include"coroutine.h"
#include"coevent.h"
#include"timewheel.h"

#include<vector>

//one loop per thread
class coloop :uncopyable {
public:
	typedef std::function<void()> functor;

	coloop();
	~coloop();

	void loop();
	void quit() { quit_ = 1; }

	class coloop_item :
		coroutine::coroutine_item, 
		public coevent {
		friend class coloop;
	public:

		coloop_item(int fd, const functor& func, coloop* loop);
		coloop_item(int fd, functor&& func, coloop* loop);
		~coloop_item();

		void setTimeout(unsigned int ms);//仅限在协程执行过程中调用
		inline void updateEvents();

	private:

		static void coroutineFunc(coloop_item* cpt);

		coloop* loop_;
		klinknode<coloop_item*> timenode_;
		functor Func;

	};

private:

	void add(coloop_item* cpt);
	void modify(coloop_item* cpt);
	void remove(coloop_item* cpt);

	bool looping_;
	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	timewheel<coloop_item*> time_wheel_;

};

void coloop::coloop_item::updateEvents() {
	loop_->modify(this);
}