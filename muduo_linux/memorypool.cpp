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

	if (queue_.empty())
		makeSpace();
	temp_ = queue_.front();
	queue_.pop();

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

	queue_.push(temp_);

	pthread_mutex_unlock(&lock_);
}

void memorypool::makeSpace() {
	addr head_;
	allocator<tcpconnection>* conn_ = new allocator<tcpconnection>;
	allocator<channel>* ch_ = new allocator<channel>;
	head_.conn_ = conn_->allocate(size_);
	head_.ch_ = ch_->allocate(size_);
	head_map_[head_] = size_;
	for (int i = 0; i < size_; ++i) {
		queue_.push(head_);
		head_.conn_ += 1;
		head_.ch_ += 1;
	}
	size_ *= 2;
}