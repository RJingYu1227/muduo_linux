﻿#pragma once

#include"eventloop.h"
#include"channel.h"
#include"buffer.h"
#include"memorypool.h"
#include<netinet/in.h>
#include<functional>
#include<string>

class eventloop;
class channel;
class buffer;
class memorypool;

class tcpconnection :public std::enable_shared_from_this<tcpconnection> {
	friend class memorypool;
public:
	typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
	//const tcpconn_ptr 指的是指针的值是一个常量 tcpconnection* const
	typedef std::function<void(const tcpconn_ptr&)> event_callback;

	tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr);
	~tcpconnection();

	static void deleter(tcpconnection* conn);

	void setConnCallback(const event_callback& cb) { newConnCallback = cb; }
	void setCloseCallback(const event_callback& cb) { closeConnCallback = cb; }
	void setWriteCallback(const event_callback& cb) { writeCompleteCallback = cb; }
	void setMsgCallback(const event_callback& cb) { recvMsgCallback = cb; }

	//tcp选项，注意线程安全
	void setTcpNoDelay(bool on);
	void setTcpKeepAlive(bool on);

	//channel选项，如果在该tcpconn的回调函数中调用，那么xxxInLoop则是不必要的
	void startRead();
	void stopRead();
	void sendBuffer(buffer* data);

	buffer* inputBuffer() { return &inbuffer_; }
	buffer* outputBuffer() { return &outbuffer_; }

	void start();
	void activeClosure();//并非立即关闭
	int fd() { return fd_; }
	//bool connected() { return state_ == 1; }
	char* getIp() { return ip_; }
	int getPort() { return port_; }

	int state_;

private:
	//channel选项，建议使用shared_from_this()，不然不是线程安全的
	//void startReadInLoop();
	//void stopReadInLoop();
	void sendBufferInLoop(const char* data,size_t len);
	void sendBufferInLoop(const std::string &data);

	void handleRead();
	void handleClose();
	void handleWrite();
	void handleError();

	eventloop* loop_;
	int fd_;
	channel* channel_;
	char* ip_;
	int port_;
	buffer inbuffer_;
	buffer outbuffer_;

	event_callback newConnCallback;
	event_callback closeConnCallback;
	event_callback writeCompleteCallback;
	event_callback recvMsgCallback;
};

