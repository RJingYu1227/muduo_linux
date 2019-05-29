#include"tcpserver.h"
#include"logging.h"

#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>

tcpserver::tcpserver(eventloop* serverloop, const char* ip, int port, int loopnum)
	:ip_(ip),
	port_(port),
	listenfd_(socket(AF_INET, SOCK_STREAM, 0)),
	serverloop_(serverloop),
	lpool_(new elthreadpool(serverloop, loopnum)) {

	bindFd();
	mpool1_ = new memorypool<tcpconnection>(1024);
	mpool2_ = new memorypool<channel>(1024);
	channel_ = new channel(serverloop_, listenfd_);
	channel_->setReadCallback(std::bind(&tcpserver::acceptConn, this));
	listening_ = 0;

	LOG << "创建TcpServer：" << ip_ << ' ' << port_ << ' ' << listenfd_;
}

tcpserver::~tcpserver() {
	channel_->remove();
	delete channel_;
	close(listenfd_);

	for (auto& temp : connections_) {
		tcpconn_ptr conn = temp.second;
		conn->loop_->runInLoop(std::bind(&tcpconnection::froceDestory, conn.get()));
	}
	delete lpool_;

	connections_.clear();
	delete mpool1_;
	delete mpool2_;

	LOG << "关闭TcpServer：" << ip_ << ' ' << port_ << ' ' << listenfd_;
}

void tcpserver::start() {
	serverloop_->assertInLoopThread();

	if (listen(listenfd_, SOMAXCONN) == -1) {
		LOG << "TcpServer监听失败，errno = " << errno;
		exit(1);
	}
	channel_->enableReading();
	lpool_->start();
	listening_ = 1;

	LOG << "TcpServer开始监听";
}

void tcpserver::bindFd() {
	if (listenfd_ == -1) {
		LOG << "创建listenfd失败，errno = " << errno;
		exit(1);
	}

	sockaddr_in serveraddr_;
	bzero(&serveraddr_, sizeof serveraddr_);
	serveraddr_.sin_family = AF_INET;
	//serveraddr_.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, ip_, &serveraddr_.sin_addr);
	serveraddr_.sin_port = htons(static_cast<uint16_t>(port_));

	if (bind(listenfd_, (sockaddr*)&serveraddr_, sizeof serveraddr_) == -1) {
		LOG << "绑定监听端口失败，errno = " << errno;
		exit(1);
	}
}

void tcpserver::acceptConn() {
	sockaddr_in cliaddr_;
	socklen_t cliaddrlen_ = sizeof cliaddr_;
	int clifd_ = accept(listenfd_, (sockaddr*)&cliaddr_, &cliaddrlen_);
	if (clifd_ == -1) {
		LOG << "建立连接失败，errno = " << errno;
		return;
	}

	eventloop* ioloop_ = lpool_->getLoop();
	tcpconnection* conn_;
	channel* ch_;
	mpool1_->setPtr(conn_);
	mpool2_->setPtr(ch_);
	new(ch_)channel(ioloop_, clifd_);
	new(conn_)tcpconnection(ioloop_, ch_, clifd_, &cliaddr_);

	tcpconn_ptr new_(conn_, std::bind(&tcpserver::deleter, this, std::placeholders::_1));
	new_->setConnectedCallback(connectedCallback);
	new_->setClosedCallback(std::bind(&tcpserver::removeConn, this, std::placeholders::_1));
	new_->setRecvDoneCallback(recvDoneCallback);
	new_->setSendDoneCallback(sendDoneCallback);

	ioloop_->queueInLoop(std::bind(&tcpconnection::start, new_));
	connections_.emplace(clifd_, new_);
}

void tcpserver::deleter(tcpconnection* conn) {
	if (serverloop_->isInLoopThread())
		deleterInLoop(conn);
	else
		serverloop_->queueInLoop(std::bind(&tcpserver::deleterInLoop, this, conn));
	conn = nullptr;
}

void tcpserver::deleterInLoop(tcpconnection* conn) {
	close(conn->getFd());
	mpool2_->destroyPtr(conn->channel_);
	mpool1_->destroyPtr(conn);
}

void tcpserver::removeConn(const tcpconn_ptr &conn) {
	serverloop_->runInLoop(std::bind(&tcpserver::removeConnInLoop, this, conn));
}

void tcpserver::removeConnInLoop(const tcpconn_ptr &conn) {
	closedCallback(conn);

	connections_.erase(conn->getFd());
}
