#pragma once
#include"tcpconnection.h"
#include"channel.h"
#include<memory>
#include<queue>
#include<pthread.h>
#include<map>
using namespace::std;

class channel;
class tcpconnection;

struct  addr {
	tcpconnection* conn_ = nullptr;
	channel* ch_ = nullptr;
};

class memorypool//这是在堆上申请空间，地址从低到高
{
public:
	memorypool(int init = 64);
	~memorypool();

	channel* setAddr(tcpconnection* &conn);
	void deleteConn(tcpconnection* conn);
	int size() { return size_; }

private:
	void makeSpace();

	pthread_mutex_t lock_;
	int size_;

	queue<addr> queue_;
};

