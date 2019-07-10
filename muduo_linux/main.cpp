#include"logging.h"

#include"httpserver.h"
#include"httprequest.h"
#include"httpresponse.h"

#include"tcpserver.h"
#include"tcpconnection.h"


#include<iostream>

void onConnected(const tcpconn_ptr& conn){

}

void onRecvDone(const tcpconn_ptr& conn) {
	buffer* buff = conn->getRecvBuffer();
	conn->send(buff);
	buff->retrieveAll();
}

void onClosed(const tcpconn_ptr& conn) {
	
}

void onSendDone(const tcpconn_ptr& conn) {
	conn->shutDown();
}

void httpCallback(const httprequest& request, httpresponse& response) {
	if (request.getPath() == "/hello") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "text/plain");
		response.addHeader("Server", "Muduo");
		response.getBody() = "hello, world!\n";
	}
	else {
		response.setStatu1(httpresponse::k404NotFound);
		response.setStatu2("Not Found");
		response.setKeepAlive(0);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 4)
		return 0;

	logger::createAsyncLogging();
	tcpconnection::ignoreSigPipe();

	/*tcpserver server(argv[1], atoi(argv[2]));

	server.setConnectedCallback(onConnected);
	server.setClosedCallback(onClosed);
	server.setRecvDoneCallback(onRecvDone);
	server.setSendDoneCallback(onSendDone);

	server.start();*/

	httpserver server(argv[1], atoi(argv[2]), atoi(argv[3]));
	server.setHttpCallback(httpCallback);
	server.start();

	logger::deleteAsyncLogging();

	return 0;
}

