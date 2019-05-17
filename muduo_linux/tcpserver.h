#pragma once

#include"elthreadpool.h"
#include"eventloop.h"
#include"channel.h"
#include"memorypool.h"
#include"tcpconnection.h"
#include<netinet/in.h>
#include<functional>
#include<unordered_map>

class elthreadpool;
class eventloop;
class channel;
class tcpconnection;
template class memorypool<tcpconnection>;
template class memorypool<channel>;

typedef std::shared_ptr<tcpconnection> tcpconn_ptr;

class tcpserver {
public:
	typedef std::function<void(const tcpconn_ptr&)> event_callback;//客户端事件回调

	tcpserver(elthreadpool* loop, const char* ip, int port);
	~tcpserver();

	eventloop* getLoop() { return server_loop_; }
	void setConnCallback(const event_callback& cb) { newConnCallback = cb; }
	void setCloseCallback(const event_callback& cb) { closeConnCallback = cb; }
	void setMsgCallback(const event_callback& cb) { recvMsgCallback = cb; }
	void setWriteCallback(const event_callback& cb) { writeCompleteCallback = cb; }
	bool listening() { return listening_; }
	void start();

private:
	typedef std::unordered_map<int, tcpconn_ptr> conn_map;
	void removeConn(const tcpconn_ptr &conn);
	void removeConnInLoop(const tcpconn_ptr &conn);
	void acceptConn();
	void deleter(tcpconnection* conn);
	void deleterInLoop(tcpconnection* conn);

	memorypool<tcpconnection>* mpool_1_;
	memorypool<channel>* mpool_2_;
	elthreadpool* loop_pool_;
	eventloop* server_loop_;
	int listenfd_;
	sockaddr_in serveraddr_;
	channel* channel_;
	conn_map conns_;
	bool listening_;

	event_callback newConnCallback;
	event_callback closeConnCallback;
	event_callback recvMsgCallback;
	event_callback writeCompleteCallback;
};

