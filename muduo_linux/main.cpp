#include<iostream>
#include<strings.h>
#include"elthreadpool.h"
#include"tcpserver.h"
#include<set>
#include<pthread.h>

std::set<const tcpconn_ptr> clients_;
elthreadpool test;
tcpserver server(test.getServerLoop(), "127.0.0.1", 6666);
//pthread_mutex_t lock_;

void on_connection(const tcpconn_ptr conn){
	std::cout << "收到一个新连接" << conn->getIp() << " " << conn->getPort() << std::endl;
	//pthread_mutex_lock(&lock_);
	//clients_.insert(conn);
	//pthread_mutex_unlock(&lock_);
}

void on_message(const tcpconn_ptr conn, buffer* buff, ssize_t len) {
	std::cout << buff->peek() << std::endl;
	//conn->active_closure();
	//pthread_mutex_lock(&lock_);
	//for (const tcpconnection* cli_ : clients_)
	//	if (conn->connected())
		//	send(cli_->fd(), buff, len, MSG_NOSIGNAL);//会造成Broken pipe
	//pthread_mutex_unlock(&lock_);
}

void on_closeclient(const tcpconn_ptr conn) {
	//pthread_mutex_lock(&lock_);
	//clients_.erase(conn);
	//pthread_mutex_unlock(&lock_);
}

int main() {

	server.setIoLoop(test.getIoLoop());
	server.setConnCallback(on_connection);
	server.setCloseCallback(on_closeclient);
	server.setMsgCallback(on_message);
	server.start();	

	test.start();
}

