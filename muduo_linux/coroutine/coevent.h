#pragma once

#include"ksocket.h"

#include<sys/epoll.h>

class coevent :public ksocket {
public:

	coevent(int fd, sockaddr_in& addr) :
		ksocket(fd, addr),
		events_(0),
		revents_(0) {

	}
	coevent(const char* ip, int port) :
		ksocket(ip, port),
		events_(0),
		revents_(0) {

	}

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

	uint32_t getRevents()const { return revents_; }

protected:

	uint32_t getEvents()const { return events_; }
	void setRevents(uint32_t revents) { revents_ = revents; }

private:

	enum event {
		NONE = 0,
		READ = EPOLLIN | EPOLLPRI,
		WRITE = EPOLLOUT,
		ET = EPOLLET,
	};

	uint32_t events_;
	uint32_t revents_;

};