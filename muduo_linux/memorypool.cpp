#include "memorypool.h"
#include<assert.h>

memorypool::memorypool(int init) {
	size_ = init;
	makeSpace();
	size_ /= 2;
}

memorypool::~memorypool(){
	/*assert(addr_queue_.size() == size_);
	head temp_;
	while (!head_queue_.empty()) {
		temp_ = head_queue_.front();
		head_queue_.pop();
		conn_.deallocate(temp_.conn_, temp_.size_);
		ch_.deallocate(temp_.ch_, temp_.size_);
	}*/
}

void memorypool::setAddr(tcpconnection* &conn, channel* &ch) {
	addr temp_;
	if (addr_queue_.empty())
		makeSpace();
	temp_ = addr_queue_.front();
	addr_queue_.pop();
	conn = temp_.conn_;
	ch = temp_.ch_;
}

void memorypool::destroyConn(tcpconnection* conn) {
	addr temp_;
	temp_.conn_ = conn;
	temp_.ch_ = conn->channel_;

	conn_.destroy(temp_.conn_);
	ch_.destroy(temp_.ch_);
	//析构之后再收回地址
	addr_queue_.push(temp_);

}

void memorypool::makeSpace() {
	addr addr_;
	addr_.conn_ = conn_.allocate(size_);
	addr_.ch_ = ch_.allocate(size_);

	head head_(addr_, size_);
	head_queue_.push(head_);
	for (int i = 0; i < size_; ++i) {
		addr_queue_.push(addr_);
		addr_.conn_ += 1;
		addr_.ch_ += 1;
	}
	size_ *= 2;
}