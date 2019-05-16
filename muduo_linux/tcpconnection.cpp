#include"tcpconnection.h"
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<assert.h>
#include<arpa/inet.h>

tcpconnection::tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr)
	:state_(0),
	fd_(fd),
	port_(cliaddr->sin_port),
	ip_(inet_ntoa(cliaddr->sin_addr)),
	loop_(loop),
	channel_(ch) {
	new(channel_)channel(loop_, fd_);
	channel_->setReadCallback(std::bind(&tcpconnection::handleRead, this));
	channel_->setCloseCallback(std::bind(&tcpconnection::handleClose, this));
	channel_->setWriteCallback(std::bind(&tcpconnection::handleWrite, this));
	channel_->setErrorCallback(std::bind(&tcpconnection::handleError, this));
}
//列表初始化顺序应与类内声明顺序一致

tcpconnection::~tcpconnection() {
	assert(state_ == 3);
	channel_->~channel();
}

void tcpconnection::start() {
	assert(state_ == 0);
	state_ = 1;
	channel_->enableReading();
	newConnCallback(shared_from_this());
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
	if (state_ == 1)
		loop_->runInLoop(std::bind(&tcpconnection::handleClose, shared_from_this()));
	//在执行该函数前，tcpconnection不会被析构
	//本线程，前面的activechannel调用了后面activechannel所属的tcpconn的这个函数，那么就GG了
	//但这只是妄想，哈哈
}

void tcpconnection::sendBuffer(buffer* data) {
	if (loop_->isInLoopThread())
		sendBufferInLoop(data->beginPtr(), data->usedBytes());
	else {
		void (tcpconnection::*fp)(const std::string &) = &tcpconnection::sendBufferInLoop;
		//注意
		loop_->queueInLoop(std::bind(fp, shared_from_this(), data->toString()));
	}
}

void tcpconnection::sendBufferInLoop(const std::string &data) {
	sendBufferInLoop(&data[0], data.size());
}

void tcpconnection::sendBufferInLoop(const char* data, size_t len) {
	if (state_ == 3)
		return;

	ssize_t nwrote = 0;
	size_t remaing = 0;
	bool senderror = 0;
	if (!channel_->isWriting() && outbuffer_.usedBytes() == 0) {
		nwrote = send(fd_, data, len, MSG_NOSIGNAL);
		if (nwrote >= 0) {
			remaing = len - nwrote;
			if (remaing == 0 && writeCompleteCallback) {
				loop_->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
				return;
			}
		}
		else {
			perror("发送数据时出错");
			senderror = 1;
		}
	}

	if (!senderror) {
		outbuffer_.append(data + nwrote, remaing);
		if (!channel_->isWriting())
			channel_->enableWriting();
	}
}

void tcpconnection::handleRead() {
	size_t n = inbuffer_.readFd(fd_);
	if (n > 0)
		recvMsgCallback(shared_from_this());
	else if (n == 0)
		handleClose();
}

void tcpconnection::handleClose() {
	if (state_ == 1 || state_ == 2) {
		state_ = 3;
		channel_->remove();//注意
		closeConnCallback(shared_from_this());
	}
}

void tcpconnection::handleWrite() {
	if (outbuffer_.usedBytes() != 0) {
		ssize_t nwrote = send(fd_, outbuffer_.beginPtr(), outbuffer_.usedBytes(), MSG_NOSIGNAL);
		if (nwrote >= 0) {
			outbuffer_.retrieve(nwrote);
			if (outbuffer_.usedBytes() == 0) {
				channel_->disableWrting();
				if (writeCompleteCallback)
					loop_->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
			}
		}
		else
			perror("数据发送错误");
	}
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