#ifndef TCPSERVER_H
#define TCPSERVER_H

#include"elthreadpool.h"
#include"eventloop.h"
#include"channel.h"
#include"tcpconnection.h"
#include"buffer.h"
#include<netinet/in.h>
#include<functional>
#include<pthread.h>

class elthreadpool;
class eventloop;
class channel;
class tcpconnection;
class buffer;

//typedef std::shared_ptr<tcpconnection> tcpconnection;

class tcpserver {
public:
	typedef std::function<void(tcpconnection* conn)> event_callback;//客户端事件回调
	typedef std::function<void(tcpconnection* conn, buffer* data, ssize_t len)> msg_callback;

	tcpserver(elthreadpool* loop, const char* ip, int port);
	~tcpserver();

	void setConnCallback(const event_callback& cb) { conn_callback_ = cb; }
	void setCloseCallback(const event_callback& cb) { close_callback_ = cb; }
	void setMsgCallback(const msg_callback& cb) { msg_callback_ = cb; }
	bool listening() { return listening_; }
	void start();

private:
	//typedef std::map<int, tcpconnection*> conn_map;
	//void removeConn(tcpconnection* conn);
	void acceptConn();

	elthreadpool* pool_;
	eventloop* loop_;
	int listenfd_;
	sockaddr_in serveraddr_;
	channel* channel_;
	//conn_map conns_;
	bool listening_;
	//pthread_mutex_t lock_;

	event_callback conn_callback_;
	event_callback close_callback_;
	msg_callback msg_callback_;
};

#endif // !ACCEPTOR_H

