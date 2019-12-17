#include"coevent.h"
#include"coroutine.h"

#include<unistd.h>

/*
如果fd被epoll监听，fd只要有事件发生内核就会将其加入epoll内部的就绪队列
epoll在epoll_wait时再去调用tcp_poll函数获得fd上发生的所有事件
然后只返回感兴趣的事件，对于水平触发，会再次将其加入就绪队列
*/
ssize_t coevent::read(void* buf, size_t nbytes, int ms) {
	ssize_t nread = 0;

	nread = ::read(ksocket::getFd(), buf, nbytes);
	if (ms == 0)
		return nread;

	//未出错则再读一次
	if (nread < 0 && errno == EAGAIN) {
		bool changed = 0;
		uint32_t last_events = events_;
		if (events_ != READ) {
			events_ = READ;
			updateEvents();

			changed = 1;
		}

		yield(ms);//超时或者被可读事件唤醒，所以并不需要循环读
		nread = ::read(ksocket::getFd(), buf, nbytes);

		if (changed) {
			events_ = last_events;
			updateEvents();
		}
	}

	return nread;
}

ssize_t coevent::write(const void* buf, size_t nbytes, int ms) {
	ssize_t nwrote;
	const char* src = (const char*)buf;

	nwrote = ::write(ksocket::getFd(), src, nbytes);
	if (nwrote > 0) {
		src += nwrote;
		nbytes -= nwrote;
	}
	else if (nwrote < 0) {
		if (errno != EAGAIN)
			return 0;//出错
		else
			nwrote = 0;
	}

	if (ms == 0 || nbytes == 0)//非阻塞调用或者一次性写完
		return nwrote;

	bool error = 0;
	bool changed = 0;
	uint32_t last_events = events_;
	if (events_ != READ) {
		events_ = READ;
		updateEvents();

		changed = 1;
	}

	while (nbytes) {
		yield(ms);
		nwrote = ::write(ksocket::getFd(), src, nbytes);

		//此时nworte值不可能为0
		if (nwrote > 0) {
			src += nwrote;
			nbytes -= nwrote;
		}
		else if (errno != EAGAIN) {
			error = 1;//出错
			break;
		}

		if (ms > 0)//低速调用
			break;
	}

	if (changed) {
		events_ = last_events;
		updateEvents();
	}

	return error ? (const char*)buf - src : src - (const char*)buf;
}