#include"tcpserver.h"
#include"logging.h"
#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>

tcpserver::tcpserver(elthreadpool* pool, const char* ip, int port) {
	loops_ = pool;
	serverloop_ = pool->getServerLoop();

	listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd_ == -1) {
		LOG << "创建listenfd失败";
		exit(1);
	}
	bzero(&serveraddr_, sizeof serveraddr_);
	serveraddr_.sin_family = AF_INET;
	serveraddr_.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, ip, &serveraddr_.sin_addr);
	serveraddr_.sin_port = htons(static_cast<uint16_t>(port));
	if (bind(listenfd_, (sockaddr*)&serveraddr_, sizeof serveraddr_) == -1) {
		LOG << "绑定监听端口失败";
		exit(1);
	}

	mpool1_ = new memorypool<tcpconnection>(1024);
	mpool2_ = new memorypool<channel>(1024);

	channel_ = new channel(serverloop_, listenfd_);
	channel_->setReadCallback(std::bind(&tcpserver::acceptConn, this));
	listening_ = 0;
}

tcpserver::~tcpserver() {
	channel_->remove();
	delete channel_;
	delete mpool1_;
	delete mpool2_;
	close(listenfd_);
}

void tcpserver::start() {
	serverloop_->assertInLoopThread();
	listen(listenfd_, SOMAXCONN);
	channel_->enableReading();
	listening_ = 1;
}

void tcpserver::acceptConn() {
	sockaddr_in cliaddr_;
	socklen_t cliaddrlen_ = sizeof cliaddr_;
	int clifd_ = accept(listenfd_, (sockaddr*)&cliaddr_, &cliaddrlen_);
	if (clifd_ == -1) {
		LOG << "建立连接失败";
		return;
	}

	eventloop* ioloop_ = loops_->getIoLoop();
	tcpconnection* conn_;
	channel* ch_;
	mpool1_->setPtr(conn_);
	mpool2_->setPtr(ch_);
	new(ch_)channel(ioloop_, clifd_);
	new(conn_)tcpconnection(ioloop_, ch_, clifd_, &cliaddr_);

	tcpconn_ptr new_(conn_, std::bind(&tcpserver::deleter, this, std::placeholders::_1));
	new_->setMsgCallback(recvMsgCallback);
	new_->setConnCallback(newConnCallback);
	new_->setWriteCallback(writeCompleteCallback);
	new_->setCloseCallback(std::bind(&tcpserver::removeConn, this, std::placeholders::_1));

	ioloop_->queueInLoop(std::bind(&tcpconnection::start, new_));
	conns_.emplace(clifd_, new_);
}

void tcpserver::deleter(tcpconnection* conn) {
	if (serverloop_->isInLoopThread())
		deleterInLoop(conn);
	else
		serverloop_->queueInLoop(std::bind(&tcpserver::deleterInLoop, this, conn));
	conn = nullptr;
}

void tcpserver::deleterInLoop(tcpconnection* conn) {
	mpool2_->destroyPtr(conn->channel_);
	mpool1_->destroyPtr(conn);
}

void tcpserver::removeConn(const tcpconn_ptr &conn) {
	serverloop_->runInLoop(std::bind(&tcpserver::removeConnInLoop, this, conn));
}

void tcpserver::removeConnInLoop(const tcpconn_ptr &conn) {
	closeConnCallback(conn);
	conns_.erase(conn->fd());
	close(conn->fd());
}
