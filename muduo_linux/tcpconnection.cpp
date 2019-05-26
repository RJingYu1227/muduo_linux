#include"tcpconnection.h"
#include"logging.h"
#include<sys/socket.h>
#include<unistd.h>
#include<assert.h>
#include<arpa/inet.h>
#include<string>

tcpconnection::tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr)
	:state_(0),
	fd_(fd),
	port_(cliaddr->sin_port),
	ip_(inet_ntoa(cliaddr->sin_addr)),
	loop_(loop),
	channel_(ch) {
	channel_->setReadCallback(std::bind(&tcpconnection::handleRead, this));
	channel_->setCloseCallback(std::bind(&tcpconnection::handleClose, this));
	channel_->setWriteCallback(std::bind(&tcpconnection::handleWrite, this));
	channel_->setErrorCallback(std::bind(&tcpconnection::handleError, this));
}
//列表初始化顺序应与类内声明顺序一致

tcpconnection::~tcpconnection() {
	assert(state_ == 3);
}

void tcpconnection::start() {
	assert(state_ == 0);
	state_ = 1;
	channel_->enableReading();

	LOG << "建立一个新连接 " << ip_ << ' ' << port_;
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

void tcpconnection::activeClosureWithDelay(double seconds) {
	if (state_ == 1)
		loop_->runAfter(std::bind(&tcpconnection::handleClose, shared_from_this()), seconds);
		
}

void tcpconnection::sendBuffer(buffer* data) {
	if (loop_->isInLoopThread())
		sendBufferInLoop2(data->beginPtr(), data->usedBytes());
	else
		loop_->queueInLoop(std::bind(&tcpconnection::sendBufferInLoop1, shared_from_this(), data->toString()));
}

void tcpconnection::sendBufferInLoop1(const std::string &data) {
	sendBufferInLoop2(&data[0], data.size());
}

void tcpconnection::sendBufferInLoop2(const char* data, size_t len) {
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
				LOG << "完整发送一次信息" << ip_ << ' ' << port_;
				loop_->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
				return;
			}
		}
		else {
			LOG << "发送信息时出错" << ip_ << ' ' << port_;
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
	if (n > 0) {
		recvMsgCallback(shared_from_this());
		LOG << "接收信息" << ip_ << ' ' << port_;
	}
	else if (n == 0)
		handleClose();
	else
		LOG << "接收信息时出错" << ip_ << ' ' << port_;
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
					LOG << "完整发送一次信息" << ip_ << ' ' << port_;
					loop_->queueInLoop(std::bind(writeCompleteCallback, shared_from_this()));
			}
		}
		else
			LOG << "发送信息时出错" << ip_ << ' ' << port_;
	}
}

void tcpconnection::handleError() {
	if (state_ == 1)
		LOG << "Tcp连接出错" << ip_ << ' ' << port_;
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