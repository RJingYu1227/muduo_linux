#include<iostream>
#include<string>
#include<pthread.h>
#include"elthreadpool.h"
#include"tcpserver.h"

void onNewConn(const tcpconn_ptr& conn){
	std::cout << "收到一个连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void onRecvMsg(const tcpconn_ptr& conn) {
	buffer* buff = conn->inputBuffer();
	std::cout << buff->toString() << std::endl;
	conn->sendBuffer(buff);
	buff->retrieveAll();
	conn->activeClosureWithDelay(6.666);
	//conn->activeClosure();
}

void onCloseConn(const tcpconn_ptr& conn) {
	std::cout << "断开一个连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void onWriteMsg(const tcpconn_ptr& conn) {
	std::cout << "信息已发送" << std::endl;
	//conn->activeClosureWithDelay(6.666);
}

int main() {
	elthreadpool test(4);
	tcpserver server(&test, "127.0.0.1", 6666);

	server.setConnCallback(onNewConn);
	server.setCloseCallback(onCloseConn);
	server.setMsgCallback(onRecvMsg);
	server.setWriteCallback(onWriteMsg);

	server.start();	
	test.start();
}

