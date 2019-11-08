﻿#include"coservice.h"

#include<assert.h>
#include<unistd.h>

void coservice::coservice_item::coroutineFunc(coservice_item* cst) {
	cst->Func();

	cst->service_->remove(cst);
}

coservice::coservice_item::coservice_item(int fd, const functor& func, coservice* service) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	service_(service),
	Func(func),
	inqueue_(0) {

	service_->add(this);
}

coservice::coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	service_(service),
	Func(std::move(func)),
	inqueue_(0) {

	service_->add(this);
}

coservice::coservice_item::~coservice_item() {
	if (getState() != coroutine_item::DONE)
		service_->remove(this);
}

coservice::coservice() :
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128) {

	queue_.put_back(nullptr);
	assert(epfd_ > 0);
}

coservice::~coservice() {
	close(epfd_);
}

size_t coservice::run() {
	size_t count = 0;
	coservice_item* task;
	int numevents;

	while (1) {
		task = queue_.take_front();
		if (task == nullptr) {
			if (queue_.empty())
				numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), -1);
			else
				numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 0);

			if (numevents > 0) {
				for (int i = 0; i < numevents; ++i) {
					task = (coservice_item*)revents_[i].data.ptr;
					if (task->inqueue_)
						continue;

					task->inqueue_ = 1;
					task->setRevents(revents_[i].events);
					queue_.put_back(task);
				}

				if (static_cast<size_t>(numevents) == revents_.size())
					revents_.resize(revents_.size() * 2);
			}

			queue_.put_back(nullptr);
		}
		else if (task->getState() != coroutine::coroutine_item::DONE) {
			//当一个协程执行完毕时，会将inqueue_置为0，此时有可能会再次被添加进queue_
			//此时执行resume是不被允许的
			task->resume();
			task->inqueue_ = 0;
		}
		++count;
	}

	return count;
}

void coservice::add(coservice_item* cst) {
	epoll_event ev;
	ev.events = cst->getEvents();
	ev.data.ptr = cst;
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void coservice::modify(coservice_item* cst) {
	epoll_event ev;
	ev.events = cst->getEvents();
	ev.data.ptr = cst;
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

void coservice::remove(coservice_item* cst) {
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
}