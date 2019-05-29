#include"eventloop.h"
#include"tcpserver.h"
#include"logging.h"

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

int main() {
	logger::createAsyncLogging();
	tcpconnection::ignoreSigPipe();
	eventloop* loop = new eventloop();
	tcpserver* server = new tcpserver(loop, "127.0.0.1", 6666, 2);

	server->setConnectedCallback(onConnected);
	server->setClosedCallback(onClosed);
	server->setRecvDoneCallback(onRecvDone);
	server->setSendDoneCallback(onSendDone);

	server->start();	
	loop->loop();

	delete server;
	delete loop;
	logger::deleteAsyncLogging();

	return 0;
}

