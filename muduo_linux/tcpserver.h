#pragma once

#include"memorypool.h"
#include"channel.h"
#include"uncopyable.h"

#include<netinet/in.h>
#include<functional>
#include<unordered_map>

class elthreadpool;
class eventloop;
class tcpconnection;

typedef std::shared_ptr<tcpconnection> tcpconn_ptr;

class tcpserver :uncopyable {
public:
	typedef std::function<void(const tcpconn_ptr&)> event_callback;

	tcpserver(const char* ip, int port, int loopnum = 2);
	~tcpserver();

	eventloop* getLoop() { return serverloop_; }
	bool isListening()const { return listening_; }
	void start();
	void stop();

	void setConnectedCallback(const event_callback& cb) { connectedCallback = cb; }
	void setClosedCallback(const event_callback& cb) { closedCallback = cb; }
	void setRecvDoneCallback(const event_callback& cb) { recvDoneCallback = cb; }
	void setSendDoneCallback(const event_callback& cb) { sendDoneCallback = cb; }

	int getListenFd()const { return listenfd_; }
	const char* getIp()const { return ip_; }
	int getPort()const { return port_; }

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
	elthreadpool* looppool_;
	memorypool<tcpconnection> mpool_;
	channel channel_;
	conn_map connections_;

	event_callback connectedCallback;
	event_callback closedCallback;
	event_callback recvDoneCallback;
	event_callback sendDoneCallback;

	bool listening_;

};

