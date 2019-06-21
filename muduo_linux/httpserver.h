#pragma once

#include"tcpserver.h"
#include"httprequest.h"
#include"httpresponse.h"
#include<functional>

class httprequest;
class httpresponse;

class httpserver {
public:
	typedef std::function<void(const httprequest&, httpresponse&)> callback;

	httpserver(const char* ip, int port, int loopnum = 2);

	void setHttpCallback(const callback& cb) { httpCallback = cb; }
	eventloop* getLoop();
	void start();

private:

	static void defaultCallback(const httprequest& request, httpresponse& response);
	static void onConnected(const tcpconn_ptr& conn);
	static void onClosed(const tcpconn_ptr& conn);
	void onRecvDone(const tcpconn_ptr& conn);

	tcpserver server_;
	callback httpCallback;

};