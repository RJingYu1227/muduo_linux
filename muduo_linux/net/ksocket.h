#pragma once

#include"uncopyable.h"

#include<netinet/tcp.h>
#include<arpa/inet.h>

class ksocket :uncopyable {
public:

	ksocket(const char* ip, int port);
	ksocket(int fd, sockaddr_in& addr);
	~ksocket();

	int getFd()const { return fd_; }
	uint16_t getPort()const { return htons(addr_.sin_port); }
	uint32_t getAddr1()const { return addr_.sin_addr.s_addr; }
	const char* getAddr2()const { return buf; }
	bool getTcpInfo(tcp_info* info)const;

	void bind();
	void listen();
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
	socklen_t len_;
	char buf[16];

};

