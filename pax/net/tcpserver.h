#pragma once

#include<pax/base/memorypool.h>
#include<pax/base/socket.h>

#include<pax/net/channel.h>

#include<unordered_map>

namespace pax {

class elthreadpool;
class eventloop;
class tcpconnection;

typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
typedef std::function<void(const tcpconn_ptr&)> event_callback;

class tcpserver :uncopyable {
public:

	tcpserver(const char* ip, int port, int loopnum = 2);
	~tcpserver();//析构函数不能在loop循环内执行

	eventloop* getLoop() { return serverloop_; }
	bool isListening()const { return listening_; }
	void start();
	void stop();

	void setConnectedCallback(const event_callback& cb) { connectedCallback = cb; }
	void setClosedCallback(const event_callback& cb) { closedCallback = cb; }
	void setRecvDoneCallback(const event_callback& cb) { recvDoneCallback = cb; }
	void setSendDoneCallback(const event_callback& cb) { sendDoneCallback = cb; }

	int getFd()const { return socket_.getFd(); }
	uint16_t getPort()const { return socket_.getPort(); }
	uint32_t getAddr1()const { return socket_.getAddr1(); }
	const char* getAddr2()const { return socket_.getAddr2(); }

private:

	void acceptConn();
	void removeConn(const tcpconn_ptr &conn);
	void removeConnInLoop(const tcpconn_ptr &conn);
	void deleter(tcpconnection* conn);
	void deleterInLoop(tcpconnection* conn);
	void stopInLoop();

	eventloop* serverloop_;
	elthreadpool* looppool_;

	socket socket_;
	memorypool<tcpconnection> mpool_;
	channel channel_;
	std::unordered_map<int, tcpconn_ptr> connections_;

	event_callback connectedCallback;
	event_callback closedCallback;
	event_callback recvDoneCallback;
	event_callback sendDoneCallback;

	event_callback removeFunc;
	std::function<void(tcpconnection*)> deleterFunc;

	bool listening_;

};

}//namespace pax