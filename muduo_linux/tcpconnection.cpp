#include"tcpconnection.h"
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<assert.h>
#include<arpa/inet.h>

void tcpconnection::deleter(tcpconnection* conn) {
	conn->loop_->destoryConn(conn);
	conn = nullptr;
}

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
	assert(state_ == 3);
	channel_->~channel();
	close(fd_);
}

void tcpconnection::start() {
	assert(state_ == 0);
	state_ = 1;
	channel_->enableReading();
	conn_callback_(shared_from_this());
}

void tcpconnection::startRead() {
	if (state_ == 1)
		channel_->enableReading();
}

void tcpconnection::stopRead() {
	if (state_ == 1)
		channel_->disableReading();
}

void tcpconnection::activeClosure() {
	if (state_ == 1) {
		state_ = 2;
		loop_->runInLoop(std::bind(&tcpconnection::handleClose, shared_from_this()));
	}
	//在执行该函数前，tcpconnection不会被析构
	//本线程，前面的activechannel调用了后面activechannel所属的tcpconn的这个函数，那么就GG了
	//但这只是妄想，哈哈
}

void tcpconnection::sendBuffer() {
	if (state_ == 1) {
		if (output_buff_.usedBytes() == 0) {
			perror("输出缓冲区为空");
			return;
		}
		ssize_t nwrote = send(fd_, output_buff_.beginPtr(), output_buff_.usedBytes(), MSG_NOSIGNAL);
		if (nwrote >= 0) {
			output_buff_.retrieve(nwrote);
			if (output_buff_.usedBytes() != 0)
				sendBuffer();
		}
		else
			perror("发送数据时出错");
	}
}

void tcpconnection::handleRead() {
	if (state_ == 1) {
		size_t n = input_buff_.readFd(fd_);
		if (n > 0)
			msg_callback_(shared_from_this(), &input_buff_, n);
		else if (n == 0)
			handleClose();
	}
}

void tcpconnection::handleClose() {
	if (state_ == 1 || state_ == 2) {
		state_ = 3;
		channel_->remove();//注意
		close_callback_(shared_from_this());
	}
}

void tcpconnection::handleWrite() {

}

void tcpconnection::handleError() {
	if (state_ == 1)
		perror("tcp连接出现错误");
}

void tcpconnection::setTcpNoDelay(bool on) {
	int optval = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
		&optval, static_cast<socklen_t>(sizeof optval));
}

void tcpconnection::setTcpKeepAlive(bool on) {
	int optval = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
		&optval, static_cast<socklen_t>(sizeof optval));
}