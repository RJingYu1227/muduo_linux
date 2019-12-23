#pragma once

#include<pax/base/uncopyable.h>

#include<arpa/inet.h>

//<netinet/tcp.h>
struct tcp_info;

namespace pax {

class socket :uncopyable {
public:

	socket();
	socket(int fd, const sockaddr_in& addr);

	virtual ~socket();

	bool valid()const { return state_ != INVALID; }

	bool bind(const char* ip, uint16_t port, bool server);

	int connect();

	bool listen();
	int accept(sockaddr_in* peeraddr);

	int getFd()const { return fd_; }
	const sockaddr_in& getAddr()const { return addr_; }
	const char* getIp()const { return ip_; }
	uint16_t getPort()const { return port_; }
	bool getTcpInfo(tcp_info* info)const;

	bool shutdownWrite();
	bool shutdownRead();
	bool shutdownAll();

	bool setReuseAddr(bool on);
	bool setReusePort(bool on);
	bool setTcpNodelay(bool on);
	bool setKeepalive(bool on);

private:
	enum state {
		INVALID,
		OPENED,
		BINDED,
		CONNECTING, CONNECTED, 
		LISTENING
	};

	int fd_;
	state state_;

	sockaddr_in addr_;
	char ip_[INET_ADDRSTRLEN];
	uint16_t port_;

};

inline bool socket::shutdownWrite() {
	return shutdown(fd_, SHUT_WR) == 0;
}

inline bool socket::shutdownRead() {
	return shutdown(fd_, SHUT_RD) == 0;
}

inline bool socket::shutdownAll() {
	return shutdown(fd_, SHUT_RDWR) == 0;
}

}//namespace pax