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

	tcpserver(eventloop* serverloop, const char* ip, int port, int loopnum = 2);
	~tcpserver();

	eventloop* getLoop() { return serverloop_; }
	bool isListening()const { return listening_; }
	void start();

	void setConnectedCallback(const event_callback& cb) { connectedCallback = cb; }
	void setClosedCallback(const event_callback& cb) { closedCallback = cb; }
	void setRecvDoneCallback(const event_callback& cb) { recvDoneCallback = cb; }
	void setSendDoneCallback(const event_callback& cb) { sendDoneCallback = cb; }

private:
	typedef std::unordered_map<int, tcpconn_ptr> conn_map;

	void bindFd();
	void acceptConn();
	void removeConn(const tcpconn_ptr &conn);
	void removeConnInLoop(const tcpconn_ptr &conn);
	void deleter(tcpconnection* conn);
	void deleterInLoop(tcpconnection* conn);

	const char* ip_;
	const int port_;
	const int listenfd_;

	eventloop* serverloop_;
	elthreadpool* lpool_;
	memorypool<tcpconnection>* mpool1_;
	memorypool<channel>* mpool2_;
	channel* channel_;
	conn_map connections_;

	event_callback connectedCallback;
	event_callback closedCallback;
	event_callback recvDoneCallback;
	event_callback sendDoneCallback;

	bool listening_;
};

