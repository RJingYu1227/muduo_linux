#include"eventqueue.h"
#include"eventloop.h"
#include"channel.h"

#include<unistd.h>
#include<assert.h>

eventqueue::eventqueue(eventloop* loop)
	:fd_(eventfd(0, EFD_CLOEXEC)),
	count_(0),
	loop_(loop),
	channel_(loop_, fd_) {

	assert(fd_ > 0);
	channel_.setReadCallback(std::bind(&eventqueue::handleRead, this));
	channel_.enableReading();
}

eventqueue::~eventqueue(){
	channel_.remove();
	close(fd_);
}

void eventqueue::wakeup() {
	eventfd_write(fd_, 1);
}

void eventqueue::addFunctor(const functor& func) {
	{
		klock<kmutex> x(&lock_);
		functors_.push_back(func);
	}
	eventfd_write(fd_, 1);
}

void eventqueue::addFunctor(functor&& func) {
	{
		klock<kmutex> x(&lock_);
		functors_.push_back(std::move(func));
	}
	eventfd_write(fd_, 1);
}

void eventqueue::getFunctors(std::vector<functor>& vec) {
	klock<kmutex> x(&lock_);
	vec.swap(functors_);
}

void eventqueue::handleRead() {
	eventfd_read(fd_, &count_);
}