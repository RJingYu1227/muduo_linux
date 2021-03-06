﻿#include<pax/log/logging.h>

#include<pax/net/tcpconnection.h>
#include<pax/net/eventloop.h>
#include<pax/net/channel.h>

#include<unistd.h>
#include<assert.h>

using namespace pax;

tcpconnection::tcpconnection(eventloop* loop, int fd, sockaddr_in& cliaddr)
	:loop_(loop),
	ptr_(0),
	fd_(fd),
	state_(kConnecting),
	watermark_(64 * 1024 * 1024),
	socket_(fd, cliaddr),
	channel_(loop_, fd_) {

	socket_.setTcpNodelay(1);
	channel_.setReadCallback(
		std::bind(&tcpconnection::handleRead, this));
	channel_.setWriteCallback(
		std::bind(&tcpconnection::handleWrite, this));
	channel_.setErrorCallback(
		std::bind(&tcpconnection::handleError, this));
	channel_.setCloseCallback(
		std::bind(&tcpconnection::handleClose, this));
	LOG << "建立一个新连接：" << socket_.getIp() << ' ' << socket_.getPort();
}

tcpconnection::~tcpconnection() {
	assert(state_ == kDisConnected);
}

void tcpconnection::start() {
	assert(state_ == kConnecting);
	state_ = kConnected;
	channel_.enableReading();
	connectedCallback(shared_from_this());
}

void tcpconnection::startRead() {
	if (state_ == kConnected)
		loop_->runInLoop(std::bind(&tcpconnection::startReadInLoop, 
			shared_from_this()));
}

void tcpconnection::stopRead() {
	if (state_ == kConnected)
		loop_->runInLoop(std::bind(&tcpconnection::stopReadInLoop, 
			shared_from_this()));
}

void tcpconnection::forceClose() {
	if (state_ == kConnected)
		loop_->runInLoop(std::bind(&tcpconnection::handleClose, 
			shared_from_this()));
}

void tcpconnection::forceCloseWithDelay(double seconds) {
	if (state_ == kConnected)
		loop_->runAfter(std::bind(&tcpconnection::handleClose, 
			shared_from_this()), 
			seconds);
}

void tcpconnection::shutDown() {
	if (state_ == kConnected)
		loop_->runInLoop(std::bind(&tcpconnection::shutDownInLoop, 
			shared_from_this()));
}

void tcpconnection::send(buffer* data) {
	if (state_ != kConnected)
		return;

	if (loop_->isInLoopThread())
		sendInLoop2(data->beginPtr(), data->usedBytes());
	else
		loop_->queueInLoop(std::bind(&tcpconnection::sendInLoop1,
			shared_from_this(), data->toString()));
}

void tcpconnection::send(const std::string& data) {
	if (state_ != kConnected)
		return;

	if (loop_->isInLoopThread())
		sendInLoop2(&data[0], data.size());
	else
		loop_->queueInLoop(std::bind(&tcpconnection::sendInLoop1,
			shared_from_this(), data));
}

void tcpconnection::startReadInLoop() {
	if (state_ == kConnected)
		channel_.enableReading();
}

void tcpconnection::stopReadInLoop() {
	if (state_ == kConnected)
		channel_.disableReading();
}

void tcpconnection::shutDownInLoop() {
	if (state_ != kConnected || channel_.isWriting())
		return;
	state_ = kDisConnecting;

	socket_.shutdownWrite();
}

void tcpconnection::sendInLoop1(const std::string &data) {
	sendInLoop2(&data[0], data.size());
}

void tcpconnection::sendInLoop2(const char* data, size_t len) {
	if (state_ != kConnected)
		return;

	ssize_t nwrote = 0;
	size_t remaing = len;
	if (!channel_.isWriting() && buffer2_.usedBytes() == 0) {
		nwrote = write(fd_, data, len);
		if (nwrote >= 0) {
			remaing = len - nwrote;
			if (remaing == 0){
				LOG << "完整发送一次信息";
				if (sendDoneCallback)
					loop_->queueInLoop(std::bind(sendDoneCallback, 
						shared_from_this()));
				return;
			}
		}
		else {
			LOG << "发送信息时出错，errno = " << errno;
			return;
		}
	}

	if (buffer2_.usedBytes() + remaing >= watermark_ 
		&& highWaterCallback)
		loop_->queueInLoop(std::bind(highWaterCallback, 
			shared_from_this()));

	buffer2_.append(data + nwrote, remaing);
	if (!channel_.isWriting())
		channel_.enableWriting();
}

void tcpconnection::handleRead() {
	ssize_t n = buffer1_.readFd(fd_);
	if (n > 0) {
		recvDoneCallback(shared_from_this());
		LOG << "接收一次信息";
	}
	else if (n == 0)
		handleClose();
	else
		LOG << "接收信息时出错，errno = " << errno;
}

void tcpconnection::handleClose() {
	if (state_ == kDisConnected)//这里要判断是因为配合forceClose
		return;
	state_ = kDisConnected;

	channel_.remove();//注意
	closedCallback(shared_from_this());
	LOG << "关闭一个连接：" << socket_.getIp() << ' ' << socket_.getPort();
}

void tcpconnection::handleWrite() {
	if (buffer2_.usedBytes() != 0) {
		ssize_t nwrote = write(fd_, 
			buffer2_.beginPtr(), buffer2_.usedBytes());
		if (nwrote >= 0) {
			buffer2_.retrieve(nwrote);
			if (buffer2_.usedBytes() == 0) {
				channel_.disableWrting();
				if (sendDoneCallback)
					loop_->queueInLoop(std::bind(sendDoneCallback, 
						shared_from_this()));
				LOG << "完整发送一次信息";
			}
		}
		else
			LOG << "发送信息时出错，errno = " << errno;
	}
}

void tcpconnection::handleError() {
	LOG << "Tcp连接出错，errno = " << errno;
}

void tcpconnection::setTcpNodelay(bool on) {
	socket_.setTcpNodelay(on);
}

void tcpconnection::setKeepalive(bool on) {
	socket_.setKeepalive(on);
}