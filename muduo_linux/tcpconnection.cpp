#include"tcpconnection.h"
#include"logging.h"
#include<sys/socket.h>
#include<unistd.h>
#include<assert.h>
#include<arpa/inet.h>
#include<signal.h>
#include<string>
#include<errno.h>

void tcpconnection::ignoreSigPipe() {
	signal(SIGPIPE, SIG_IGN);
	LOG << "忽略SIGPIPE";
}

tcpconnection::tcpconnection(eventloop* loop, channel* ch, int fd, sockaddr_in* cliaddr)
	:state_(0),
	fd_(fd),
	port_(cliaddr->sin_port),
	ip_(inet_ntoa(cliaddr->sin_addr)),
	highwater_(64*1024*1024),
	loop_(loop),
	channel_(ch) {

	channel_->setReadCallback(std::bind(&tcpconnection::handleRead, this));
	channel_->setWriteCallback(std::bind(&tcpconnection::handleWrite, this));
	channel_->setErrorCallback(std::bind(&tcpconnection::handleError, this));
	channel_->setCloseCallback(std::bind(&tcpconnection::handleClose, this));
	LOG << "建立一个新连接，ip = " << ip_;
}
//列表初始化顺序应与类内声明顺序一致

tcpconnection::~tcpconnection() {
	assert(state_ == 3);
}

void tcpconnection::start() {
	assert(state_ == 0);
	state_ = 1;
	channel_->enableReading();
	connectedCallback(shared_from_this());
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
		loop_->runAfter(std::bind(&tcpconnection::handleClose, 
			shared_from_this()), 
			seconds);
		
}

void tcpconnection::send(buffer* data) {
	if (loop_->isInLoopThread())
		sendInLoop2(data->beginPtr(), data->usedBytes());
	else
		loop_->queueInLoop(std::bind(&tcpconnection::sendInLoop1, 
			shared_from_this(), 
			data->toString()));
}

void tcpconnection::sendInLoop1(const std::string &data) {
	sendInLoop2(&data[0], data.size());
}

void tcpconnection::sendInLoop2(const char* data, size_t len) {
	if (state_ == 3)
		return;

	ssize_t nwrote = 0;
	size_t remaing = len;
	if (!channel_->isWriting() && buffer2_.usedBytes() == 0) {
		nwrote = write(fd_, data, len);
		if (nwrote >= 0) {
			remaing = len - nwrote;
			if (remaing == 0 && sendDoneCallback) {
				LOG << "完整发送一次信息";
				loop_->queueInLoop(std::bind(sendDoneCallback, shared_from_this()));
			}
		}
		else {
			LOG << "发送信息时出错，errno = " << errno;
			return;
		}
	}

	if (remaing == 0)
		return;

	if (buffer2_.usedBytes() + remaing >= highwater_ 
		&& highWaterCallback)
		loop_->queueInLoop(std::bind(highWaterCallback, shared_from_this()));

	buffer2_.append(data + nwrote, remaing);
	if (!channel_->isWriting())
		channel_->enableWriting();
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
	if (state_ == 1 || state_ == 2) {
		state_ = 3;
		channel_->remove();//注意
		closedCallback(shared_from_this());
	}
}

void tcpconnection::handleWrite() {
	if (buffer2_.usedBytes() != 0) {
		ssize_t nwrote = write(fd_, buffer2_.beginPtr(), buffer2_.usedBytes());
		if (nwrote >= 0) {
			buffer2_.retrieve(nwrote);
			if (buffer2_.usedBytes() == 0) {
				channel_->disableWrting();
				if (sendDoneCallback)
					loop_->queueInLoop(std::bind(sendDoneCallback, shared_from_this()));
				LOG << "完整发送一次信息";
			}
		}
		else
			LOG << "发送信息时出错，errno = " << errno;
	}
}

void tcpconnection::handleError() {
	if (state_ == 1)
		LOG << "Tcp连接出错，ip = " << ip_ << " errno = " << errno;
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