#include "memorypool.h"
#include<assert.h>

memorypool::memorypool(int init){
	conn_begin_ = conn_space_.allocate(init);
	ch_begin_ = ch_space_.allocate(init);
	while (init)
		queue_.push(--init);
}

memorypool::~memorypool(){

}

channel* memorypool::newConn(tcpconnection* &conn) {
	assert(!queue_.empty());
	pthread_mutex_lock(&lock_);

	int n = queue_.front();//对queue_操作会产生竞争-冒险现象
	queue_.pop();

	pthread_mutex_unlock(&lock_);
	conn = conn_begin_ + n;
	channel* new_ch = ch_begin_ + n;
	return new_ch;
}

void memorypool::deleteConn(tcpconnection* conn) {
	int n = conn - conn_begin_;
	conn_space_.destroy(conn);//由tcpconnection的析构函数调用channel的析构函数
	pthread_mutex_lock(&lock_);

	queue_.push(n);

	pthread_mutex_unlock(&lock_);
}