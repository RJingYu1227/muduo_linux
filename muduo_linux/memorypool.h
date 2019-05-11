#pragma once
#include"tcpconnection.h"
#include"channel.h"
#include<memory>
#include<queue>
#include<unordered_map>
#include<pthread.h>
using namespace::std;

class channel;
class tcpconnection;

struct  head {
	tcpconnection* conn_ = nullptr;
	channel* ch_ = nullptr;
};

class memorypool//这是在堆上申请空间，地址从低到高
{
public:
	memorypool(int init = 64);
	~memorypool();

	channel* newConn(tcpconnection* &conn);
	void deleteConn(tcpconnection* conn);

private:
	head* makeSpace();

	pthread_mutex_t lock_;
	int size_;

	unordered_map<head*, queue<int>> map_;
};

