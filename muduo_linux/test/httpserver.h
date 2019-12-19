#pragma once

#include"net/tcpserver.h"
#include"net/tcpconnection.h"
#include"net/eventloop.h"

#include<functional>

class httprequest;
class httpresponse;

class httpserver {
public:
	typedef std::function<void(const httprequest&, httpresponse&)> callback;

	httpserver(const char* ip, int port, int loopnum = 2);

	void setHttpCallback(const callback& cb) { httpCallback = cb; }
	pax::eventloop* getLoop();
	void start(){ server_.start(); }
	void stop() { server_.stop(); }

private:

	static void defaultCallback(const httprequest& request, httpresponse& response);
	static void onConnected(const pax::tcpconn_ptr& conn);
	static void onClosed(const pax::tcpconn_ptr& conn);
	void onRecvDone(const pax::tcpconn_ptr& conn);

	pax::tcpserver server_;
	callback httpCallback;

};