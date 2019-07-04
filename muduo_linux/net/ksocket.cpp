#include"ksocket.h"
#include"logging.h"

#include<unistd.h>
#include<assert.h>

ksocket::ksocket(const char* ip, int port)
	:fd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
	server_(1) {

	assert(fd_ > 0);
	addr_.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr_.sin_addr);
	addr_.sin_port = static_cast<uint16_t>(port);
}

ksocket::ksocket(int fd, sockaddr_in& addr)
	:fd_(fd),
	addr_(addr),
	server_(0) {

	assert(fd_ > 0);
}

ksocket::~ksocket() {
	close(fd_);
}

bool ksocket::setTcpInfo(tcp_info* info)const {
	return 0;
}

void ksocket::bind() {
	if (!server_)
		return;

	if (::bind(fd_, (sockaddr*)&addr_, sizeof addr_) == -1) {
		LOG << "绑定监听端口失败，errno = " << errno;
		exit(1);
	}
}

void ksocket::listen() {
	if (!server_)
		return;

	if (::listen(fd_, SOMAXCONN) == -1) {
		LOG << "监听失败，errno = " << errno;
		exit(1);
	}
}

int ksocket::accept(sockaddr_in* peeraddr) {
	int clifd_ = accept4(fd_, (sockaddr*)peeraddr, &len_,
		SOCK_NONBLOCK | SOCK_CLOEXEC);

	if (clifd_ == -1 && errno != EAGAIN)
		LOG << "建立连接失败，errno = " << errno;

	return clifd_;
}

void ksocket::shutdownWrite() {
	shutdown(fd_, SHUT_WR);
}

void ksocket::shutdownRead() {
	shutdown(fd_, SHUT_RD);
}

void ksocket::shutdownAll() {
	shutdown(fd_, SHUT_RDWR);
}

void ksocket::setReuseAddr(bool on) {
	
}

void ksocket::setReusePort(bool on) {

}

void ksocket::setNodelay(bool on) {
	int optval = on ? 1 : 0;
	setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
		&optval, static_cast<socklen_t>(sizeof optval));
}

void ksocket::setKeepalive(bool on) {
	int optval = on ? 1 : 0;
	setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
		&optval, static_cast<socklen_t>(sizeof optval));
}