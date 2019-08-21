#pragma once

#include"coroutine.h"

#include<sys/epoll.h>
#include<vector>

struct coloop_t {
	int fd_;
	uint32_t events_;
	coroutine_t coid_;
};

class coloop :uncopyable {
public:

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;
	static const int kEpollet;

	static coloop* threadColoop();
	static void freeColoop();

	int add(coloop_t item);
	int update(coloop_t item);
	int remove(coloop_t item);

	void loop(int timeoutms);
	void quit() { quit_ = 1; }

protected:

	coloop();
	~coloop();

private:

	static kthreadlocal<coloop> thread_loop_;

	bool quit_;
	int epfd_;
	std::vector<epoll_event> revents_;

};
