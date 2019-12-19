#pragma once

#include"uncopyable.h"

#include<netinet/tcp.h>
#include<arpa/inet.h>

namespace pax {

class socket :uncopyable {
public:

	socket(const char* ip, int port);
	socket(int fd, sockaddr_in& addr);
	~socket();

	int getFd()const { return fd_; }
	uint16_t getPort()const { return htons(addr_.sin_port); }
	uint32_t getAddr1()const { return addr_.sin_addr.s_addr; }
	const char* getAddr2()const { return ip_; }
	bool getTcpInfo(tcp_info* info)const;

	bool bind();
	bool listen();
	int accept(sockaddr_in* peeraddr);

	void shutdownWrite();
	void shutdownRead();
	void shutdownAll();

	void setReuseAddr(bool on);
	void setReusePort(bool on);
	void setTcpNodelay(bool on);
	void setKeepalive(bool on);

private:

	int fd_;
	sockaddr_in addr_;
	char ip_[16];

};

}//namespace pax