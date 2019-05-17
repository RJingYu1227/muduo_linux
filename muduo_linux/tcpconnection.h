#pragma once

#include"eventloop.h"
#include"channel.h"
#include"buffer.h"
#include<memory>
#include<netinet/in.h>
#include<functional>

class eventloop;
class channel;

class tcpconnection :public std::enable_shared_from_this<tcpconnection> {
	friend class tcpserver;
public:
	typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
	//const tcpconn_ptr 指的是指针的值是一个常量 tcpconnection* const
	typedef std::function<void(const tcpconn_ptr&)> event_callback;

	tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr);
	~tcpconnection();

	eventloop* getLoop() { return loop_; }
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
	void activeClosure();
	void activeClosureWithDelay(double seconds);//秒为单位
	int fd() { return fd_; }
	//bool connected() { return state_ == 1; }
	char* getIp() { return ip_; }
	int getPort() { return port_; }

private:
	//channel选项，建议使用shared_from_this()，不然不是线程安全的
	//void startReadInLoop();
	//void stopReadInLoop();
	void sendBufferInLoop1(const std::string &data);//const左值引用
	void sendBufferInLoop2(const char* data, size_t len);

	void handleRead();
	void handleClose();
	void handleWrite();
	void handleError();

	int state_;
	int fd_;
	int port_;
	char* ip_;
	eventloop* loop_;
	channel* channel_;

	buffer inbuffer_;
	buffer outbuffer_;

	event_callback newConnCallback;
	event_callback closeConnCallback;
	event_callback writeCompleteCallback;
	event_callback recvMsgCallback;
	//成员变量声明顺序因为字节对齐原因会影响类的大小
};

