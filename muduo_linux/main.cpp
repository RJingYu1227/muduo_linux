#include<iostream>
#include<string>
#include"elthreadpool.h"
#include"tcpserver.h"
#include<pthread.h>

elthreadpool test(4);
tcpserver server(&test, "127.0.0.1", 6666);

void on_connection(tcpconnection* conn){
	std::cout << "收到一个新连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void on_message(tcpconnection* conn, buffer* buff, ssize_t len) {
	std::cout << buff->toString() << std::endl;
	buffer* out = conn->outputBuffer();
	out->append(buff->beginPtr(), len);
	out->hasUsed(len);
	buff->retrieve(len);
	conn->sendBuffer();
	conn->activeClosure();
}

void on_closeclient(tcpconnection* conn) {

}

int main() {

	server.setConnCallback(on_connection);
	server.setCloseCallback(on_closeclient);
	server.setMsgCallback(on_message);
	server.start();	

	test.start();
}

