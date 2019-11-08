#pragma once

#include"uncopyable.h"

#include<sys/epoll.h>

class coevent {
public:

	explicit coevent(int fd) :
		fd_(fd),
		events_(0),
		revents_(0) {

	}

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

	int fd_;
	uint32_t events_;
	uint32_t revents_;

};

template<typename T>
struct klinknode {

	void remove();
	void join(klinknode* phead);

	klinknode* prev_;
	T val_;
	klinknode* next_;

};

template<typename T>
void klinknode<T>::remove() {
	if (prev_)
		prev_->next_ = next_;
	if (next_)
		next_->prev_ = prev_;

	prev_ = nullptr;
	next_ = nullptr;
}

template<typename T>
void klinknode<T>::join(klinknode<T>* phead) {
	if (phead == nullptr)
		return;

	if (phead->next_) {
		next_ = phead->next_;
		phead->next_->prev_ = this;
	}
	prev_ = phead;
	phead->next_ = this;
}