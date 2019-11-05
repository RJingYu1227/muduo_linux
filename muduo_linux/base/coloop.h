#pragma once

#include"coroutine.h"

#include<sys/epoll.h>
#include<vector>
#include<list>

class coloop :uncopyable {
public:

	static uint64_t getMilliSeconds();
	inline static void loop();
	inline static void quit();

	class coloop_item :uncopyable {
		friend class coloop;
	public:

		coloop_item(coroutine_t id, int fd);
		~coloop_item();

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

		struct timeout_t {

			timeout_t* prev_;
			coloop_item* cpt_;
			timeout_t* next_;

		};

		enum coevent {
			NONE = 0,
			READ = EPOLLIN | EPOLLPRI,
			WRITE = EPOLLOUT,
			ET = EPOLLET,
		};

		coloop* loop_;
		coroutine_t id_;
		int fd_;

		uint32_t events_;
		uint32_t revents_;
		timeout_t timeout_;

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
	void setTimeout(unsigned int ms, coloop_item::timeout_t* cpt);
	void loopFunc();

	bool running_;
	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	int tindex_;
	uint64_t last_time_;
	std::vector<coloop_item::timeout_t> time_wheel_;

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
