#include<pax/base/socket.h>

#include<pax/log/logging.h>

#include<unistd.h>
#include<strings.h>
#include<assert.h>

using namespace::pax;

socket::socket(const char* ip, int port) :
	fd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, SOL_TCP)) {

	assert(fd_ > 0);
	bzero(&addr_, sizeof(sockaddr_in));

	addr_.sin_family = AF_INET;

	inet_pton(AF_INET, ip, &addr_.sin_addr);
	strncpy(ip_, ip, 16);

	addr_.sin_port = htons(static_cast<uint16_t>(port));
}

socket::socket(int fd, sockaddr_in& addr) :
	fd_(fd),
	addr_(addr) {

	assert(fd_ > 0);
	inet_ntop(AF_INET, &addr_.sin_addr, ip_, 16);
}

socket::~socket() {
	::close(fd_);
}

bool socket::getTcpInfo(tcp_info* info)const {
	socklen_t len = sizeof(tcp_info);
	bzero(info, len);
	return getsockopt(fd_, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool socket::bind() {
	if (::bind(fd_, (sockaddr*)&addr_, sizeof addr_) == -1) {
		LOG << "bind失败，errno = " << errno;
		return 0;
	}

	return 1;
}

bool socket::listen() {
	if (::listen(fd_, SOMAXCONN) == -1) {
		LOG << "listen失败，errno = " << errno;
		return 0;
	}

	return 1;
}

int socket::accept(sockaddr_in* peeraddr) {
	socklen_t len = sizeof(sockaddr_in);//值-结果参数，不初始化时，可能会出错，errno = 22
	bzero(peeraddr, len);
	int clifd_ = ::accept4(fd_, (sockaddr*)peeraddr, &len,
		SOCK_NONBLOCK | SOCK_CLOEXEC);

	if (clifd_ == -1 && errno != EAGAIN)
		LOG << "建立连接失败，errno = " << errno;

	return clifd_;
}

void socket::shutdownWrite() {
	if (shutdown(fd_, SHUT_WR) == -1)
		LOG << "shutdownWrite失败，errno = " << errno;
}

void socket::shutdownRead() {
	if (shutdown(fd_, SHUT_RD) == -1)
		LOG << "shutdownRead失败，errno = " << errno;
}

void socket::shutdownAll() {
	if (shutdown(fd_, SHUT_RDWR) == -1)
		LOG << "shutdownAll失败，errno = " << errno;
}

void socket::setReuseAddr(bool on) {
	int optval = on ? 1 : 0;
	int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
		&optval, sizeof optval);
	if (ret < 0 && on)
		LOG << "setReuseAddr失败，errno = " << errno;
}

void socket::setReusePort(bool on) {
	int optval = on ? 1 : 0;
	int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,
		&optval, sizeof optval);
	if (ret < 0 && on)
		LOG << "setReusePort失败，errno = " << errno;
}

void socket::setTcpNodelay(bool on) {
	int optval = on ? 1 : 0;
	int ret = setsockopt(fd_, SOL_TCP, TCP_NODELAY,
		&optval, sizeof optval);
	if (ret < 0 && on)
		LOG << "setTcpNodelay失败，errno = " << errno;
}

void socket::setKeepalive(bool on) {
	int optval = on ? 1 : 0;
	int ret = setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
		&optval, sizeof optval);
	if (ret < 0 && on)
		LOG << "setKeepalive失败，errno = " << errno;
}