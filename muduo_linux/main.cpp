#include<iostream>
#include<string>
#include<pthread.h>
#include"elthreadpool.h"
#include"tcpserver.h"

void on_connection(const tcpconn_ptr& conn){
	std::cout << "收到一个连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void on_message(const tcpconn_ptr& conn) {
	buffer* buff = conn->inputBuffer();
	std::cout << buff->toString() << std::endl;
	conn->sendBuffer(buff);
	buff->retrieveAll();
	//conn->activeClosure();
}

void on_closeclient(const tcpconn_ptr& conn) {
	std::cout << "断开一个连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void on_writemsg(const tcpconn_ptr& conn) {
	std::cout << "信息已发送" << std::endl;
}

int main() {
	elthreadpool test(4);

	tcpserver server(&test, "127.0.0.1", 6666);
	server.setConnCallback(on_connection);
	server.setCloseCallback(on_closeclient);
	server.setMsgCallback(on_message);
	server.setWriteCallback(on_writemsg);

	server.start();	
	test.start();
}

