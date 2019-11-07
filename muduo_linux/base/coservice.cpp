#include"coservice.h"

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
	Func(func) {

	service_->add(this);
}

coservice::coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	service_(service),
	Func(std::move(func)) {

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