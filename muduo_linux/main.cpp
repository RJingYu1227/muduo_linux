#include"elthreadpool.h"
#include"tcpserver.h"
#include"logging.h"
#include<string>
#include<pthread.h>
#include<functional>

void onNewConn(const tcpconn_ptr& conn){
	LOG << "收到一个连接 " << conn->getIp() << " " << conn->getPort();
}

void onRecvMsg(const tcpconn_ptr& conn) {
	buffer* buff = conn->inputBuffer();
	LOG << buff->toString();
	//conn->sendBuffer(buff);
	buff->retrieveAll();
	//conn->activeClosureWithDelay(6.666);
	//conn->activeClosure();
}

void onCloseConn(const tcpconn_ptr& conn) {
	LOG << "断开一个连接 " << conn->getIp() << " " << conn->getPort();
}

void onWriteMsg(const tcpconn_ptr& conn) {
	LOG << "信息已发送";
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

