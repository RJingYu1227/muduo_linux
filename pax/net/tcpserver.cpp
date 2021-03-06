﻿#include<pax/log/logging.h>

#include<pax/net/tcpserver.h>
#include<pax/net/elthreadpool.h>
#include<pax/net/eventloop.h>
#include<pax/net/tcpconnection.h>

using namespace pax;

using std::placeholders::_1;

tcpserver::tcpserver(const char* ip, int port, int loopnum)
	:serverloop_(new eventloop()),
	looppool_(new elthreadpool(loopnum)),
	socket_(),
	mpool_(1024),
	channel_(serverloop_, socket_.getFd()),
	removeFunc(std::bind(&tcpserver::removeConn, this, _1)),
	deleterFunc(std::bind(&tcpserver::deleter, this, _1)),
	listening_(0) {

	socket_.bind(ip, static_cast<uint16_t>(port), 1);
	channel_.setReadCallback(std::bind(&tcpserver::acceptConn, this));
	LOG << "创建TcpServer：" << socket_.getIp() << ' ' << socket_.getPort();
}

tcpserver::~tcpserver() {
	stop();

	for (auto& temp : connections_) {
		temp.second->setClosedCallback(closedCallback);
		temp.second->forceClose();
	}
	delete looppool_;

	connections_.clear();
	delete serverloop_;
	LOG << "关闭TcpServer：" << socket_.getIp() << ' ' << socket_.getPort();
}

void tcpserver::start() {
	if (listening_)
		return;

	looppool_->start();

	socket_.listen();
	channel_.enableReading();
	listening_ = 1;
	LOG << "TcpServer开始监听";
	serverloop_->loop();
}

void tcpserver::stop() {
	serverloop_->runInLoop(std::bind(&tcpserver::stopInLoop, this));
}

void tcpserver::stopInLoop() {
	if (!listening_)
		return;

	channel_.remove();
	listening_ = 0;
	LOG << "TcpServer停止监听";
	serverloop_->quit();
}

void tcpserver::acceptConn() {
	sockaddr_in cliaddr_;
	tcpconnection* conn_;

	while (1) {
		int clifd_ = socket_.accept(cliaddr_);
		if (clifd_ == -1)
			break;

		eventloop* ioloop_ = looppool_->getLoop();
		if (ioloop_ == nullptr)
			ioloop_ = serverloop_;

		mpool_.setPtr(conn_);
		new(conn_)tcpconnection(ioloop_, clifd_, cliaddr_);

		tcpconn_ptr new_(conn_, deleterFunc);
		new_->setConnectedCallback(connectedCallback);
		new_->setClosedCallback(removeFunc);
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