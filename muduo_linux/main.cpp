#include<iostream>
#include<strings.h>
#include"elthreadpool.h"
#include"tcpserver.h"
#include<set>
#include<pthread.h>

std::set<const tcpconn_ptr> clients_;
elthreadpool test;
tcpserver server(&test, "127.0.0.1", 6666);

void on_connection(const tcpconn_ptr conn){
	std::cout << "收到一个新连接" << conn->getIp() << " " << conn->getPort() << std::endl;
}

void on_message(const tcpconn_ptr conn, buffer* buff, ssize_t len) {
	std::cout << buff->peek() << std::endl;
	buff->retrieve(len);
}

void on_closeclient(const tcpconn_ptr conn) {

}

int main() {

	server.setConnCallback(on_connection);
	server.setCloseCallback(on_closeclient);
	server.setMsgCallback(on_message);
	server.start();	

	test.start();
}

