#pragma once
#include"tcpconnection.h"
#include"channel.h"
#include<memory>
#include<queue>
#include<initializer_list>

class channel;
class tcpconnection;

class memorypool
{
public:
	memorypool(int init = 233);
	~memorypool();

	channel* newElement(tcpconnection* &conn);
	void deleteElement(tcpconnection* element);

private:
	tcpconnection* conn_begin_;
	channel* ch_begin_;
	std::allocator<tcpconnection> conn_space_;
	std::allocator<channel> ch_space_;
	std::queue<int> queue_;
};

