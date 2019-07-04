#pragma once

#include"uncopyable.h"

#include<netinet/tcp.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string>

using std::string;

class ksocket :uncopyable {
public:

	ksocket(const char* ip, int port);
	ksocket(int fd, sockaddr_in& addr);
	~ksocket();

	int getFd()const { return fd_; }
	uint16_t getPort()const { return addr_.sin_port; }
	uint32_t getAddr()const { return addr_.sin_addr.s_addr; }
	string getIp()const { return inet_ntoa(addr_.sin_addr); }
	bool setTcpInfo(tcp_info* info)const;

	void bind();
	void listen();
	int accept(sockaddr_in* peeraddr);

	void shutdownWrite();
	void shutdownRead();
	void shutdownAll();

	void setReuseAddr(bool on);
	void setReusePort(bool on);
	void setNodelay(bool on);
	void setKeepalive(bool on);

private:

	int fd_;
	sockaddr_in addr_;
	bool server_;

	socklen_t len_ = sizeof(sockaddr_in);

};

