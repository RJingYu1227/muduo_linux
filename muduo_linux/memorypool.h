#pragma once

#include"tcpconnection.h"
#include"channel.h"
#include<memory>
#include<queue>
#include<pthread.h>
using namespace::std;

class channel;
class tcpconnection;

struct  addr {
	tcpconnection* conn_ = nullptr;
	channel* ch_ = nullptr;
};

struct head {
	head(const addr &addr_, const int &size_) {
		conn_ = addr_.conn_;
		ch_ = addr_.ch_;
		this->size_ = size_;
	}
	tcpconnection* conn_ = nullptr;
	channel* ch_ = nullptr;
	int size_ = 0;
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

	queue<head> head_queue_;
	queue<addr> addr_queue_;
};

