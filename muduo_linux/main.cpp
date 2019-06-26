#include"eventloop.h"
#include"tcpserver.h"
#include"httpserver.h"
#include"logging.h"

#include<stdlib.h>

void onConnected(const tcpconn_ptr& conn){

}

void onRecvDone(const tcpconn_ptr& conn) {
	buffer* buff = conn->getRecvBuffer();
	conn->send(buff);
	buff->retrieveAll();
	conn->shutDown();
}

void onClosed(const tcpconn_ptr& conn) {
	
}

void onSendDone(const tcpconn_ptr& conn) {
	conn->shutDown();
}

void httpCallback(const httprequest& request, httpresponse& response) {
	
}

int main(int argc, char* argv[]) {
	if (argc != 3)
		return 0;

	logger::createAsyncLogging();
	tcpconnection::ignoreSigPipe();
	/*tcpserver server("127.0.0.1", 6666, 2);

	server.setConnectedCallback(onConnected);
	server.setClosedCallback(onClosed);
	server.setRecvDoneCallback(onRecvDone);
	server.setSendDoneCallback(onSendDone);

	server.start();	*/
	httpserver server(argv[1], atoi(argv[2]), 2);

	server.setHttpCallback(httpCallback);

	server.start();
	logger::deleteAsyncLogging();

	return 0;
}

