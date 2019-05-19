#include<iostream>
#include<string>
#include<pthread.h>
#include<functional>
#include"elthreadpool.h"
#include"tcpserver.h"
using namespace std;

void onNewConn(const tcpconn_ptr& conn){
	cout << "收到一个连接" << conn->getIp() << " " << conn->getPort() << endl;
}

void onRecvMsg(const tcpconn_ptr& conn) {
	buffer* buff = conn->inputBuffer();
	std::cout << buff->toString() << std::endl;
	conn->sendBuffer(buff);
	buff->retrieveAll();
	//conn->activeClosureWithDelay(6.666);
	//conn->activeClosure();
}

void onCloseConn(const tcpconn_ptr& conn) {
	cout << "断开一个连接" << conn->getIp() << " " << conn->getPort() << endl;
}

void onWriteMsg(const tcpconn_ptr& conn) {
	cout << "信息已发送" << endl;
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

	return 0;
}

