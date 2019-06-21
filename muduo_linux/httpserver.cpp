#include"httpserver.h"
#include"logging.h"

httpserver::httpserver(const char* ip, int port, int loopnum)
	: server_(ip, port, loopnum),
	httpCallback(httpserver::defaultCallback) {

	server_.setConnectedCallback(httpserver::onConnected);
	server_.setRecvDoneCallback(std::bind(&httpserver::onRecvDone,
		this, std::placeholders::_1));
	server_.setClosedCallback(httpserver::onClosed);
}

eventloop* httpserver::getLoop() {
	return server_.getLoop();
}

void httpserver::start() {
	LOG << "Http服务器模式";
	server_.start();
}

void httpserver::defaultCallback(const httprequest& request, httpresponse& response) {
	response.setStatu1(httpresponse::k404NotFound);
	response.setStatu2("Not Found");
	response.setKeepAlive(0);
}

void httpserver::onConnected(const tcpconn_ptr& conn) {
	conn->setPtr(reinterpret_cast<unsigned long>(new httprequest()));

}

void httpserver::onRecvDone(const tcpconn_ptr& conn) {
	httprequest* request = reinterpret_cast<httprequest*>(conn->getPtr());

	if (!request->praseRequest(conn->getRecvBuffer())) {
		conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
		conn->shutDown();
	}
	else if (request->praseDone()) {
		bool alive = request->getHeader("Connection") == "Keep-Alive";
		httpresponse response(alive);
		httpCallback(*request, response);
		buffer buffer2;
		response.appendToBuffer(&buffer2);
		conn->send(&buffer2);
		if (response.keepAlive())
			request->reset();
		else
			conn->shutDown();
	}
}

void httpserver::onClosed(const tcpconn_ptr& conn) {
	delete reinterpret_cast<httprequest*>(conn->getPtr());

}

