#pragma once

#include"coroutine.h"

#include<sys/epoll.h>
#include<vector>

class coloop :uncopyable {
public:
	enum coevent {
		NONE = 0,
		READ = EPOLLIN | EPOLLPRI,
		WRITE = EPOLLOUT,
		ET = EPOLLET,
	};

	static coloop* threadColoop();
	static void freeColoop();

	int add(int fd, coevent ce, coroutine_t id);
	int modify(int fd, coevent ce, coroutine_t id);
	int remove(int fd, coevent ce, coroutine_t id);

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
