#include"elthreadpool.h"
#include"tcpserver.h"
#include"logging.h"
#include<string>
#include<pthread.h>
#include<functional>

void onNewConn(const tcpconn_ptr& conn){

}

void onRecvMsg(const tcpconn_ptr& conn) {
	buffer* buff = conn->inputBuffer();
	//conn->sendBuffer(buff);
	buff->retrieveAll();
	conn->activeClosureWithDelay(6.666);
	//conn->activeClosure();
}

void onCloseConn(const tcpconn_ptr& conn) {
	
}

void onWriteMsg(const tcpconn_ptr& conn) {
	//conn->activeClosureWithDelay(6.666);
}

int main() {
	elthreadpool test(3);
	tcpserver server(&test, "127.0.0.1", 6666);

	server.setConnCallback(onNewConn);
	server.setCloseCallback(onCloseConn);
	server.setMsgCallback(onRecvMsg);
	server.setWriteCallback(onWriteMsg);

	server.start();	
	test.start();

	return 0;
}

