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

channel* memorypool::newElement(tcpconnection* &conn) {
	assert(!queue_.empty());
	int n = queue_.front();
	queue_.pop();
	conn = conn_begin_ + n;
	channel* new_ch = ch_begin_ + n;
	return new_ch;
}

void memorypool::deleteElement(tcpconnection* element) {
	int n = element - conn_begin_;
	conn_space_.destroy(element);//由tcpconnection的析构函数调用channel的析构函数
	queue_.push(n);
}