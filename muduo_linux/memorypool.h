#pragma once
#include"tcpconnection.h"
#include"channel.h"
#include<memory>
#include<queue>

class channel;
class tcpconnection;

class memorypool
{
public:
	memorypool(int init = 233);
	~memorypool();

	channel* newConn(tcpconnection* &conn);
	void deleteConn(tcpconnection* conn);

private:
	pthread_mutex_t lock_;
	tcpconnection* conn_begin_;
	channel* ch_begin_;
	std::allocator<tcpconnection> conn_space_;
	std::allocator<channel> ch_space_;
	std::queue<int> queue_;
};

