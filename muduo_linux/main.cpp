#include"elthreadpool.h"
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
	elthreadpool test(3);
	tcpserver server(&test, "127.0.0.1", 6666);

	server.setConnectedCallback(onConnected);
	server.setClosedCallback(onClosed);
	server.setRecvDoneCallback(onRecvDone);
	server.setSendDoneCallback(onSendDone);

	server.start();	
	test.start();

	return 0;
}

