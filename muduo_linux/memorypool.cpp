#include "memorypool.h"

memorypool::memorypool(int init) {
	size_ = init;
	makeSpace();
	size_ /= 2;
}

memorypool::~memorypool(){

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

void memorypool::deleteConn(tcpconnection* conn) {
	addr temp_;
	temp_.conn_ = conn;
	temp_.ch_ = conn->channel_;
	conn->~tcpconnection();//析构之后再收回地址
	addr_queue_.push(temp_);

}

void memorypool::makeSpace() {
	addr addr_;
	allocator<tcpconnection>* conn_ = new allocator<tcpconnection>;
	allocator<channel>* ch_ = new allocator<channel>;
	addr_.conn_ = conn_->allocate(size_);
	addr_.ch_ = ch_->allocate(size_);
	delete conn_;
	delete ch_;

	head head_(addr_, size_);
	head_queue_.push(head_);
	for (int i = 0; i < size_; ++i) {
		addr_queue_.push(addr_);
		addr_.conn_ += 1;
		addr_.ch_ += 1;
	}
	size_ *= 2;
}