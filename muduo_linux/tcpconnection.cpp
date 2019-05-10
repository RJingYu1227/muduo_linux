#include"tcpconnection.h"
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<assert.h>
#include<arpa/inet.h>

tcpconnection::tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr)
	:loop_(loop),
	fd_(fd),
	state_(0),
	ip_(inet_ntoa(cliaddr->sin_addr)),
	port_(cliaddr->sin_port),
	channel_(ch) {
	new(channel_)channel(loop_, fd_);
	channel_->setReadCallback(std::bind(&tcpconnection::handleRead, this));
	channel_->setCloseCallback(std::bind(&tcpconnection::handleClose, this));
	channel_->setWriteCallback(std::bind(&tcpconnection::handleWrite, this));
	channel_->setErrorCallback(std::bind(&tcpconnection::handleError, this));
}

tcpconnection::~tcpconnection() {
	assert(state_ == 2);
	state_ = 3;
	channel_->~channel();
	close(fd_);
}

void tcpconnection::start() {
	assert(state_ == 0);
	channel_->enableReading();
	state_ = 1;
	conn_callback_(this);
}

void tcpconnection::activeClosure() {
	handleClose();
	perror("服务器主动断开一个连接");
}

void tcpconnection::handleRead() {
	if (state_ == 1) {
		loop_->assertInLoopThread();
		size_t n = input_buff_.readFd(fd_);
		if (n > 0)
			msg_callback_(this, &input_buff_, n);
		else if (n == 0) {
			handleClose();
			perror("客户端主动断开一个连接");
		}
	}
}

void tcpconnection::handleClose() {
	if (state_ == 1 && (state_ = 2))
		close_callback_(this);
}

void tcpconnection::handleWrite() {

}

void tcpconnection::handleError() {
	perror("tcp连接出现错误");
}
