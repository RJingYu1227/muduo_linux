#include"tcpserver.h"
#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<errno.h>

tcpserver::tcpserver(elthreadpool* pool, const char* ip, int port)
	:pool_(pool), 
	listening_(0){
	server_loop_ = pool_->getServerLoop();

	listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd_ == -1) {
		perror("创建listenfd失败");
		exit(1);
	}
	bzero(&serveraddr_, sizeof serveraddr_);
	serveraddr_.sin_family = AF_INET;
	serveraddr_.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, ip, &serveraddr_.sin_addr);
	serveraddr_.sin_port = htons(port);
	if (bind(listenfd_, (sockaddr*)&serveraddr_, sizeof serveraddr_) == -1) {
		perror("绑定监听端口失败");
		exit(1);
	}

	channel_ = new channel(server_loop_, listenfd_);
	channel_->setReadCallback(std::bind(&tcpserver::acceptConn,this));
	//返回时tcp三次握手已经完成，双方都已经确认连接（系统调用），存在syn队列和accept队列
}

tcpserver::~tcpserver() {
	delete channel_;
	close(listenfd_);
}

void tcpserver::start() {
	server_loop_->assertInLoopThread();
	listen(listenfd_, SOMAXCONN);
	channel_->enableReading();
	listening_ = 1;
}

void tcpserver::acceptConn() {
	sockaddr_in cliaddr_;
	socklen_t cliaddrlen_=sizeof cliaddr_;
	int clifd_ = accept(listenfd_, (sockaddr*)&cliaddr_, &cliaddrlen_);
	if (clifd_ == -1) {
		perror("建立连接失败");
		exit(1);
	}

	eventloop* ioloop_ = pool_->getIoLoop();
	tcpconnection* temp_;
	ioloop_->newConn(temp_, clifd_, &cliaddr_);

	tcpconn_ptr new_(temp_, tcpconnection::deleter);
	new_->setMsgCallback(msg_callback_);
	new_->setConnCallback(conn_callback_);
	new_->setCloseCallback(std::bind(&tcpserver::removeConn, this, std::placeholders::_1));
	ioloop_->runInLoop(std::bind(&tcpconnection::start, new_));

	conns_[clifd_] = new_;
}

void tcpserver::removeConn(const tcpconn_ptr &conn) {
	server_loop_->runInLoop(std::bind(&tcpserver::removeConnInLoop, this, conn));
}

void tcpserver::removeConnInLoop(const tcpconn_ptr &conn) {
	close_callback_(conn);
	conns_.erase(conn->fd());
}
