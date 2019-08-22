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
	static uint64_t getMilliSeconds();

	int add(int fd, int ce, coroutine_t id);
	int modify(int fd, int ce, coroutine_t id);
	int remove(int fd);
	void runAfter(unsigned int ms, coroutine_t id);

	void loop();
	void quit() { quit_ = 1; }

protected:

	coloop();
	~coloop();

private:

	static kthreadlocal<coloop> thread_loop_;

	bool quit_;

	int epfd_;
	std::vector<epoll_event> revents_;

	int tindex_;
	uint64_t last_time_;
	std::vector<std::vector<coroutine_t>> time_wheel_;

};