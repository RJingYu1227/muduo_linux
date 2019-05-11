#include "memorypool.h"

memorypool::memorypool(int init) {
	size_ = init;
	makeSpace();
	size_ /= 2;
}

memorypool::~memorypool(){

}

channel* memorypool::newConn(tcpconnection* &conn) {
	pthread_mutex_lock(&lock_);//应该还可以继续缩小临界区

	for (auto iter = map_.begin(); iter != map_.end(); ++iter) {
		if (iter->second.empty())
			continue;
		else {
			int n = iter->second.front();
			iter->second.pop();
			conn = iter->first->conn_ + n;
			channel* new_ch = iter->first->ch_ + n;

			pthread_mutex_unlock(&lock_);//
			return new_ch;
		}
	}
	head* temp_ = makeSpace();
	int n = map_[temp_].front();
	map_[temp_].pop();

	pthread_mutex_unlock(&lock_);
	conn = temp_->conn_ + n;
	channel* new_ch = temp_->ch_ + n;
	return new_ch;
	
}

void memorypool::deleteConn(tcpconnection* conn) {
	head* begin_;
	pthread_mutex_lock(&lock_);

	for (auto iter = map_.begin(); iter != map_.end(); ++iter) {
		if (iter->first->conn_ < conn) {
			begin_ = iter->first;
			++iter;
			for (iter; iter != map_.end(); ++iter) {
				if (iter->first->conn_<conn && iter->first->conn_>begin_->conn_)
					begin_ = iter->first;
				else if (iter->first->conn_ == conn) {
					begin_ = iter->first;
					break;
				}
			}
			break;
		}
		else if (iter->first->conn_ == conn) {
			begin_ = iter->first;
			break;
		}
	}//找到对应的起始地址，内存块的数目不会很多，因为是指数增长的
	int n = conn - begin_->conn_;
	conn->~tcpconnection();
	conn = nullptr;
	map_[begin_].push(n);

	pthread_mutex_unlock(&lock_);
}

head* memorypool::makeSpace() {
	head* begin_ = new head();
	allocator<tcpconnection>* conn_ = new allocator<tcpconnection>;
	allocator<channel>* ch_ = new allocator<channel>;
	begin_->conn_ = conn_->allocate(size_);
	begin_->ch_ = ch_->allocate(size_);
	for (int i = 0; i < size_; ++i)
		map_[begin_].push(i);
	size_ *= 2;
	return begin_;
}