#include"tcpserver.h"
#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<errno.h>

tcpserver::tcpserver(eventloop* loop, const char* ip, int port) :loop_(loop), listening_(0) {
	listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd_ == -1) {
		perror("创建listenfd失败");
		exit(1);
	}
	bzero(&serveraddr_, sizeof serveraddr_);
	serveraddr_.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &serveraddr_.sin_addr);
	serveraddr_.sin_port = htons(port);
	if (bind(listenfd_, (sockaddr*)&serveraddr_, sizeof serveraddr_) == -1) {
		perror("绑定监听端口失败");
		exit(1);
	}
	channel_ = new channel(loop_, listenfd_);
	channel_->setReadCallback(std::bind(&tcpserver::acceptConn,this));
	//返回时tcp三次握手已经完成，双方都已经确认连接（系统调用），存在syn队列和accept队列
}

tcpserver::~tcpserver() {
	delete channel_;
	close(listenfd_);
}

void tcpserver::start() {
	loop_->assertInLoopThread();
	listening_ = 1;
	listen(listenfd_, SOMAXCONN);
	channel_->enableReading();
}

void tcpserver::acceptConn() {
	loop_->assertInLoopThread();
	sockaddr_in cliaddr_;
	socklen_t cliaddrlen_=sizeof cliaddr_;
	int clifd_ = accept(listenfd_, (sockaddr*)&cliaddr_, &cliaddrlen_);
	if (clifd_ == -1) {
		perror("建立连接失败");
		exit(1);
	}
	tcpconn_ptr new_conn(new tcpconnection(ioloop_, clifd_, &cliaddr_));
	new_conn->setMsgCallback(msg_callback_);
	new_conn->setConnCallback(conn_callback_);
	new_conn->setCloseCallback(std::bind(&tcpserver::removeConn, this, std::placeholders::_1));
	ioloop_->runInLoop(std::bind(&tcpconnection::start, new_conn));//
	pthread_mutex_lock(&lock_);

	conns_[clifd_] = new_conn;

	pthread_mutex_unlock(&lock_);
}

void tcpserver::removeConn(const tcpconn_ptr conn) {
	ioloop_->assertInLoopThread();
	close_callback_(conn);//为了线程安全
	pthread_mutex_lock(&lock_);

	conns_.erase(conn->fd());

	pthread_mutex_unlock(&lock_);
	//线程安全，因为是io线程调用的
}
