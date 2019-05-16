#pragma once

#include"tcpconnection.h"
#include"channel.h"
#include<queue>
#include<memory>
using namespace::std;

class channel;
class tcpconnection;

struct  addr {
	tcpconnection* conn_ = nullptr;
	channel* ch_ = nullptr;
};

struct head {
	head() {}
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

	void setAddr(tcpconnection* &conn, channel* &ch);
	void destroyConn(tcpconnection* conn);
	int size() { return size_; }

private:
	void makeSpace();

	int size_;
	allocator<tcpconnection> conn_;
	allocator<channel> ch_;

	queue<head> head_queue_;
	queue<addr> addr_queue_;
};

