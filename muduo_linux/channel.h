﻿#pragma once

#include"eventloop.h"
#include<functional>

class eventloop;

class channel {
public:
	typedef std::function<void()> event_callback;

	channel(eventloop* loop, int fd);
	~channel();

	void handleEvent();

	void setReadCallback(const event_callback& cb) { readCallback = cb; }
	void setWriteCallback(const event_callback& cb) { writeCallback = cb; }
	void setErrorCallback(const event_callback& cb) { errorCallback = cb; }
	void setCloseCallback(const event_callback& cb) { closeCallback = cb; }

	int fd()const { return fd_; }
	int getEvent()const { return event_; }
	void setRevent(int revt) { revent_ = revt; }
	bool isNoneEvent()const { return event_ == kNoneEvent; }
	bool isWriting() const { return event_ & kWriteEvent; }
	bool isReading() const { return event_ & kReadEvent; }
	eventloop* ownerLoop() { return loop_; }

	//修改event_
	void enableReading() { event_ |= kReadEvent; update(); }
	void enableWriting() { event_ |= kWriteEvent; update(); }
	void disableWrting() { event_ &= ~kWriteEvent; update(); }
	void disableReading() { event_ &= kReadEvent; update(); }
	void disableALL() { event_ = kNoneEvent; update(); }

	//epoller
	int mark() { return mark_; }
	void setMark(int mark_) { this->mark_ = mark_; }
	void remove();

private:
	void update();

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	eventloop* loop_;
	int fd_;
	int event_;
	int revent_;
	int mark_;

	event_callback readCallback;
	event_callback writeCallback;
	event_callback errorCallback;
	event_callback closeCallback;
};

