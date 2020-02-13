#include<pax/base/socket.h>

#include<netinet/tcp.h>
#include<unistd.h>
#include<assert.h>
#include<string.h>
#include<errno.h>

using namespace::pax;

socket::socket() :
	fd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, SOL_TCP)),
	state_(OPENED) {

	addr_.sin_family = AF_INET;
	if (fd_ < 0)
		state_ = INVALID;
}

socket::socket(int fd, const sockaddr_in& addr) :
	fd_(fd),
	state_(CONNECTED),
	addr_(addr),
	port_(ntohs(addr.sin_port)) {

	assert(fd_ > 0);
	inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip_, INET_ADDRSTRLEN);
}

socket::~socket() {
	if (state_ != INVALID)
		::close(fd_);
}

bool socket::bind(const char* ip, uint16_t port, bool server) {
	assert(state_ == OPENED);

	if (inet_pton(AF_INET, ip, &addr_.sin_addr) == 1) {
		//该函数成功返回1，出错返回0
		addr_.sin_port = htons(port);

		if (server == false || ::bind(fd_, (sockaddr*)&addr_, sizeof addr_) == 0) {
			strncpy(ip_, ip, INET_ADDRSTRLEN);
			port_ = port;
			state_ = BINDED;

			return 1;
		}
	}

	return 0;
}

/*
本方法通过重复调用connect，判断是否连接成功
不太肯定会不会有EINTR错误
*/
int socket::connect() {
	assert(state_ == BINDED || state_ == CONNECTING);

	if (::connect(fd_, (sockaddr*)&addr_, sizeof(sockaddr_in)) == 0 || errno == EISCONN) {
		state_ = CONNECTED;
		return 1;
	}

	if (errno == EINPROGRESS) {
		state_ = CONNECTING;
		return 0;
	}
	else {
		::close(fd_);
		fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, SOL_TCP);
		if (fd_ < 0)
			state_ = INVALID;
		else
			state_ = OPENED;

		return -1;
	}
}

bool socket::listen() {
	assert(state_ == BINDED);

	if (::listen(fd_, SOMAXCONN) == 0) {
		state_ = LISTENING;
		return 1;
	}

	return 0;
}

int socket::accept(sockaddr_in& peeraddr) {
	assert(state_ == LISTENING);

	socklen_t len = sizeof(sockaddr_in);//值-结果参数，不初始化时，可能会出错，errno = 22
	bzero(&peeraddr, len);
	int clifd = ::accept4(fd_, (sockaddr*)&peeraddr, &len,
		SOCK_NONBLOCK | SOCK_CLOEXEC);

	if (clifd > 0)
		return clifd;

	if (errno == EAGAIN)
		return 0;
	else
		return -1;
}

bool socket::getTcpInfo(tcp_info* info)const {
	socklen_t len = sizeof(tcp_info);
	bzero(info, len);
	return getsockopt(fd_, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool socket::setReuseAddr(bool on) {
	int optval = on ? 1 : 0;
	return setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == 0;
}

bool socket::setReusePort(bool on) {
	int optval = on ? 1 : 0;
	return setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) == 0;
}

bool socket::setTcpNodelay(bool on) {
	int optval = on ? 1 : 0;
	return setsockopt(fd_, SOL_TCP, TCP_NODELAY, &optval, sizeof optval) == 0;
}

bool socket::setKeepalive(bool on) {
	int optval = on ? 1 : 0;
	return setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval) == 0;
}
