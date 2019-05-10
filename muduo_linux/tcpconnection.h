#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include"eventloop.h"
#include"channel.h"
#include"buffer.h"
#include<netinet/in.h>
#include<functional>

class eventloop;
class channel;
class buffer;

class tcpconnection /*:
	public std::enable_shared_from_this<tcpconnection>*/ {
public:
	//typedef std::shared_ptr<tcpconnection> tcpconnection;//const tcpconn_ptr 指的是指针的值是一个常量 tcpconnection* const
	//enum state { kconnecting = 0, kconnected, kdisconnecting, kdisconnected };
	//typedef std::function<void(tcpconnection* conn)> conn_callback;
	typedef std::function<void(tcpconnection* conn)> event_callback;
	typedef std::function<void(tcpconnection* conn, buffer* data, ssize_t len)> msg_callback;

	tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr);
	~tcpconnection();

	void setConnCallback(const event_callback& cb) { conn_callback_ = cb; }
	void setCloseCallback(const event_callback& cb) { close_callback_ = cb; }
	void setMsgCallback(const msg_callback& cb) { msg_callback_ = cb; }

	//tcp
	//void set_tcpnodelay();

	void start();
	void activeClosure();
	int fd() { return fd_; }
	bool connected() { return state_ == 1; }
	char* getIp() { return ip_; }
	int getPort() { return port_; }

	int state_;

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

#endif // ! TCPCONNECTION_H

