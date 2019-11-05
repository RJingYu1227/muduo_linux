#pragma once

#include"coroutine.h"

#include<sys/epoll.h>
#include<vector>

template<typename T>
struct klinknode {

	void remove();
	void join(klinknode* phead);

	klinknode* prev_;
	T val_;
	klinknode* next_;

};

class coloop :uncopyable {
public:
	typedef std::function<void()> functor;

	static uint64_t getMilliSeconds();
	inline static void loop();
	inline static void quit();

	class coloop_item :public coroutine::coroutine_item {
		friend class coloop;
	public:

		coloop_item(int fd, const functor& func);
		coloop_item(int fd, functor&& func);
		~coloop_item();

		int getFd()const { return fd_; }

		void enableReading() { events_ |= READ; }
		void disableReading() { events_ &= ~READ; }
		bool isReading()const { return events_ & READ; }

		void enableWriting() { events_ |= WRITE; }
		void disableWrting() { events_ &= ~WRITE; }
		bool isWriting()const { return events_ & WRITE; }

		void enableEpollet() { events_ |= ET; }
		void disableEpollet() { events_ &= ~ET; }
		bool isEpollet()const { return events_ & ET; }

		void disableALL() { events_ = NONE; }
		bool isNoneEvent()const { return events_ == NONE; }

		inline void setTimeout(unsigned int ms);

		inline void updateEvents();
		uint32_t getRevents()const { return revents_; }

	private:

		static void coroutineFunc(coloop_item* cpt);

		enum coevent {
			NONE = 0,
			READ = EPOLLIN | EPOLLPRI,
			WRITE = EPOLLOUT,
			ET = EPOLLET,
		};

		coloop* loop_;
		int fd_;

		uint32_t events_;
		uint32_t revents_;
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
	void setTimeout(unsigned int ms, klinknode<coloop_item*>* cpt);
	void loopFunc();

	bool running_;
	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	int tindex_;
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
	loop_->setTimeout(ms, &timeout_);
}

void coloop::coloop_item::updateEvents() {
	loop_->modify(this);
}