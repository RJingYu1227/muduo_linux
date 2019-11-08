#pragma once

#include"coroutine.h"
#include"coevent.h"

#include<vector>

class coloop :uncopyable {
public:
	typedef std::function<void()> functor;

	static uint64_t getMilliSeconds();
	inline static void loop();
	inline static void quit();

	class coloop_item :
		public coroutine::coroutine_item, 
		public coevent {
		friend class coloop;
	public:

		coloop_item(int fd, const functor& func);
		coloop_item(int fd, functor&& func);
		~coloop_item();

		inline void setTimeout(unsigned int ms);
		inline void cancelTimeout();
		inline void updateEvents();

	private:

		static void coroutineFunc(coloop_item* cpt);

		coloop* loop_;
		klinknode<coloop_item*> timeout_;
		functor Func;

	};

protected:

	coloop();
	~coloop();

private:

	static coloop* threadColoop();
	static void freeColoop(void* ptr);

	static kthreadlocal<coloop> thread_loop_;

	void add(coloop_item* cpt);
	void modify(coloop_item* cpt);
	void remove(coloop_item* cpt);
	void setTimeout(unsigned int ms, klinknode<coloop_item*>* timeout);
	void cancelTimeout(klinknode<coloop_item*>* timeout);
	void loopFunc();

	bool running_;
	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	size_t tindex_;
	size_t count_;
	uint64_t last_time_;
	std::vector<klinknode<coloop_item*>> time_wheel_;

};

void coloop::loop() {
	threadColoop()->loopFunc();
}

void coloop::quit() {
	threadColoop()->quit_ = 1;
}

void coloop::coloop_item::setTimeout(unsigned int ms) {
	timeout_.val_ = this;
	loop_->setTimeout(ms, &timeout_);
}

void coloop::coloop_item::cancelTimeout() {
	loop_->cancelTimeout(&timeout_);
}

void coloop::coloop_item::updateEvents() {
	loop_->modify(this);
}