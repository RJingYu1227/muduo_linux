﻿#pragma once

#include"eventloop.h"
#include"channel.h"
#include"buffer.h"
#include"memorypool.h"
#include<netinet/in.h>
#include<functional>
#include<atomic>

class eventloop;
class channel;
class buffer;
class memorypool;

class tcpconnection :public std::enable_shared_from_this<tcpconnection> {
	friend class memorypool;
public:
	typedef std::shared_ptr<tcpconnection> tcpconn_ptr;//const tcpconn_ptr 指的是指针的值是一个常量 tcpconnection* const
	typedef std::function<void(const tcpconn_ptr)> event_callback;
	typedef std::function<void(const tcpconn_ptr, buffer*, ssize_t)> msg_callback;

	tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr);
	~tcpconnection();

	static void deleter(tcpconnection* conn);

	void setConnCallback(const event_callback& cb) { conn_callback_ = cb; }
	void setCloseCallback(const event_callback& cb) { close_callback_ = cb; }
	void setMsgCallback(const msg_callback& cb) { msg_callback_ = cb; }

	//tcp选项
	void setTcpNoDelay(bool on);
	void setTcpKeepAlive(bool on);

	//channel选项
	void startRead();
	void stopRead();
	void sendBuffer();

	buffer* inputBuffer() { return &input_buff_; }
	buffer* outputBuffer() { return &output_buff_; }

	void start();
	void activeClosure();//并非立即关闭
	int fd() { return fd_; }
	bool connected() { return state_ == 1; }
	char* getIp() { return ip_; }
	int getPort() { return port_; }

	std::atomic<int> state_;//

private:
	void handleRead();
	void handleClose();
	void handleWrite();
	void handleError();

	eventloop* loop_;
	int fd_;
	channel* channel_;
	char* ip_;
	int port_;
	buffer input_buff_;
	buffer output_buff_;

	event_callback conn_callback_;
	event_callback close_callback_;
	msg_callback msg_callback_;
};

