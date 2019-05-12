#include "memorypool.h"

memorypool::memorypool(int init) {
	size_ = init;
	makeSpace();
	size_ /= 2;
}

memorypool::~memorypool(){

}

channel* memorypool::setAddr(tcpconnection* &conn) {
	addr temp_;
	pthread_mutex_lock(&lock_);

	if (addr_queue_.empty())
		makeSpace();
	temp_ = addr_queue_.front();
	addr_queue_.pop();

	pthread_mutex_unlock(&lock_);
	conn = temp_.conn_;
	return temp_.ch_;
	
}

void memorypool::deleteConn(tcpconnection* conn) {
	addr temp_;
	temp_.conn_ = conn;
	temp_.ch_ = conn->channel_;
	conn->~tcpconnection();
	pthread_mutex_lock(&lock_);

	addr_queue_.push(temp_);

	pthread_mutex_unlock(&lock_);
}

void memorypool::makeSpace() {
	addr addr_;
	allocator<tcpconnection>* conn_ = new allocator<tcpconnection>;
	allocator<channel>* ch_ = new allocator<channel>;
	addr_.conn_ = conn_->allocate(size_);
	addr_.ch_ = ch_->allocate(size_);

	head head_(addr_, size_);
	head_queue_.push(head_);
	for (int i = 0; i < size_; ++i) {
		addr_queue_.push(addr_);
		addr_.conn_ += 1;
		addr_.ch_ += 1;
	}
	size_ *= 2;
}