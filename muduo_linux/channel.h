#pragma once

#include"eventloop.h"
#include<functional>

class eventloop;

class channel {
public:
	typedef std::function<void()> event_callback;

	channel(eventloop* loop, int fd);
	~channel();

	void handleEvent();

	void setReadCallback(const event_callback& cb) { read_callback_ = cb; }
	void setWriteCallback(const event_callback& cb) { write_callback_ = cb; }
	void setErrorCallback(const event_callback& cb) { error_callback_ = cb; }
	void setCloseCallback(const event_callback& cb) { close_callback_ = cb; }

	int fd()const { return fd_; }
	int getEvent()const { return event_; }
	void setRevent(int revt) { revent_ = revt; }
	bool isNoneEvent()const { return event_ == kNoneEvent; }
	bool isWriting() const { return event_ & kWriteEvent; }
	bool isReading() const { return event_ & kReadEvent; }
	eventloop* ownerLoop() { return loop_; }

	//ÐÞ¸Äevent_
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
	const int fd_;
	int event_;
	int revent_;
	int mark_;

	event_callback read_callback_;
	event_callback write_callback_;
	event_callback close_callback_;
	event_callback error_callback_;
};

