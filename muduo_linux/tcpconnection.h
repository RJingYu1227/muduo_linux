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
	typedef std::function<void(const tcpconn_ptr&)> event_callback;

	static void ignoreSigPipe();

	tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr);
	~tcpconnection();

	eventloop* getLoop() { return loop_; }
	void setConnectedCallback(const event_callback& cb) { connectedCallback = cb; }
	void setClosedCallback(const event_callback& cb) { closedCallback = cb; }
	void setRecvDoneCallback(const event_callback& cb) { recvDoneCallback = cb; }
	void setSendDoneCallback(const event_callback& cb) { sendDoneCallback = cb; }
	void setHighWaterCallback(const event_callback& cb, size_t highwater) {
		highWaterCallback = cb;
		highwater_ = highwater;
	}

	//tcp选项，注意线程安全
	void setTcpNoDelay(bool on);
	void setTcpKeepAlive(bool on);

	//channel选项，如果在该tcpconn的回调函数中调用，那么xxxInLoop则是不必要的
	void startRead();
	void stopRead();
	void send(buffer* data);

	buffer* getRecvBuffer() { return &buffer1_; }
	buffer* getSendBuffer() { return &buffer2_; }

	void start();
	void activeClosure();
	void activeClosureWithDelay(double seconds);//秒为单位
	int fd() { return fd_; }
	bool connected() { return state_ == 1; }
	char* getIp() { return ip_; }
	int getPort() { return port_; }

private:
	//channel选项，建议使用shared_from_this()，不然不是线程安全的
	//void startReadInLoop();
	//void stopReadInLoop();
	void sendInLoop1(const std::string &data);//const左值引用
	void sendInLoop2(const char* data, size_t len);

	void handleRead();
	void handleClose();
	void handleWrite();
	void handleError();

	int state_;
	int fd_;
	int port_;
	char* ip_;
	size_t highwater_;

	eventloop* loop_;
	channel* channel_;

	buffer buffer1_;
	buffer buffer2_;//这里的buffer不是指针

	event_callback connectedCallback;
	event_callback closedCallback;
	event_callback recvDoneCallback;
	event_callback sendDoneCallback;
	event_callback highWaterCallback;
	//成员变量声明顺序因为字节对齐原因会影响类的大小
};

