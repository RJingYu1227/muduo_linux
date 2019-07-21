#pragma once

#include"uncopyable.h"

#include<functional>

class eventloop;

class channel :uncopyable {
public:
	typedef std::function<void()> functor;

	channel(eventloop* loop, int fd);
	~channel();

	void handleEvent();

	void setReadCallback(const functor& func) { readCallback = func; }
	void setWriteCallback(const functor& func) { writeCallback = func; }
	void setErrorCallback(const functor& func) { errorCallback = func; }
	void setCloseCallback(const functor& func) { closeCallback = func; }

	int getFd()const { return fd_; }
	int getEvent()const { return event_; }
	eventloop* getLoop() { return loop_; }

	//event
	void enableReading() 
	{ event_ |= kReadEvent; update(); }
	void disableReading() 
	{ event_ &= ~kReadEvent; update(); }
	bool isReading() const 
	{ return event_ & kReadEvent; }

	void enableWriting() 
	{ event_ |= kWriteEvent; update(); }
	void disableWrting() 
	{ event_ &= ~kWriteEvent; update(); }
	bool isWriting() const 
	{ return event_ & kWriteEvent; }

	void disableALL() 
	{ event_ = kNoneEvent; update(); }
	bool isNoneEvent()const 
	{ return event_ == kNoneEvent; }

	//poller
	void setMark(int mark) { mark_ = mark; }
	int getMark()const { return mark_; }
	void setRevent(int revt) { revent_ = revt; }
	int getRevent()const { return revent_; }

	void remove();

private:

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	void update();

	eventloop* loop_;
	int fd_;
	int event_;
	int revent_;
	int mark_;

	functor readCallback;
	functor writeCallback;
	functor errorCallback;
	functor closeCallback;

};

