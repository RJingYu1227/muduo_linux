#include"tcpserver.h"
#include"logging.h"
#include"elthreadpool.h"
#include"eventloop.h"
#include"tcpconnection.h"

#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>

tcpserver::tcpserver(const char* ip, int port, int loopnum)
	:ip_(ip),
	port_(port),
	listenfd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
	serverloop_(new eventloop()),
	looppool_(new elthreadpool(loopnum)),
	mpool_(1024),
	channel_(serverloop_, listenfd_),
	listening_(0) {

	bindFd();
	channel_.setReadCallback(std::bind(&tcpserver::acceptConn, this));
	LOG << "创建TcpServer：" << ip_ << ' ' << port_ << ' ' << listenfd_;
}

tcpserver::~tcpserver() {
	stop();
	close(listenfd_);

	for (auto& temp : connections_) {
		temp.second->setClosedCallback(closedCallback);
		temp.second->forceClose();
	}
	delete looppool_;

	connections_.clear();
	delete serverloop_;
	LOG << "关闭TcpServer：" << ip_ << ' ' << port_ << ' ' << listenfd_;
}

void tcpserver::start() {
	if (listening_)
		return;

	serverloop_->assertInLoopThread();

	if (listen(listenfd_, SOMAXCONN) == -1) {
		LOG << "TcpServer监听失败，errno = " << errno;
		exit(1);
	}
	channel_.enableReading();

	looppool_->start();
	listening_ = 1;
	LOG << "TcpServer开始监听";
	serverloop_->loop();
}

void tcpserver::stop() {
	if (!listening_)
		return;

	serverloop_->quit();
	while (serverloop_->isLooping());

	channel_.remove();
	listening_ = 0;
}

void tcpserver::bindFd() {
	if (listenfd_ == -1) {
		LOG << "创建listenfd失败，errno = " << errno;
		exit(1);
	}

	sockaddr_in serveraddr_;
	bzero(&serveraddr_, sizeof serveraddr_);
	serveraddr_.sin_family = AF_INET;
	inet_pton(AF_INET, ip_, &serveraddr_.sin_addr);
	serveraddr_.sin_port = htons(static_cast<uint16_t>(port_));

	if (bind(listenfd_, (sockaddr*)&serveraddr_, sizeof serveraddr_) == -1) {
		LOG << "绑定监听端口失败，errno = " << errno;
		exit(1);
	}
}

void tcpserver::acceptConn() {
	sockaddr_in cliaddr_;
	socklen_t len_ = sizeof cliaddr_;
	while (1) {
		bzero(&cliaddr_, len_);
		int clifd_ = accept4(listenfd_, (sockaddr*)&cliaddr_, &len_,
			SOCK_NONBLOCK | SOCK_CLOEXEC);

		if (clifd_ == -1) {
			if (errno != EAGAIN)
				LOG << "建立连接失败，errno = " << errno;
			break;
		}

		eventloop* ioloop_ = looppool_->getLoop();
		if (ioloop_ == nullptr)
			ioloop_ = serverloop_;

		tcpconnection* conn_;
		mpool_.setPtr(conn_);
		new(conn_)tcpconnection(ioloop_, clifd_, &cliaddr_);

		tcpconn_ptr new_(conn_, std::bind(&tcpserver::deleter, this, std::placeholders::_1));
		new_->setConnectedCallback(connectedCallback);
		new_->setClosedCallback(std::bind(&tcpserver::removeConn, this, std::placeholders::_1));
		new_->setRecvDoneCallback(recvDoneCallback);
		new_->setSendDoneCallback(sendDoneCallback);

		ioloop_->runInLoop(std::bind(&tcpconnection::start, new_));
		connections_.emplace(clifd_, new_);
	}
}

void tcpserver::deleter(tcpconnection* conn) {
	serverloop_->runInLoop(std::bind(&tcpserver::deleterInLoop, this, conn));
}

void tcpserver::deleterInLoop(tcpconnection* conn) {
	mpool_.destroyPtr(conn);
}

void tcpserver::removeConn(const tcpconn_ptr &conn) {
	serverloop_->runInLoop(std::bind(&tcpserver::removeConnInLoop, this, conn));
}

void tcpserver::removeConnInLoop(const tcpconn_ptr &conn) {
	closedCallback(conn);
	connections_.erase(conn->getFd());
}
