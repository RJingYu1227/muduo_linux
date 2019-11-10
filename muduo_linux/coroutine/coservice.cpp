#include"coservice.h"

#include<assert.h>
#include<unistd.h>

thread_local coservice_item* coservice_item::running_cst_ = nullptr;

coservice_item::coservice_item(int fd, const functor& func, coservice* service) :
	coroutine_item(func),
	coevent(fd),
	service_(service),
	handling_(0) {

	service_->add(this);
}

coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::move(func)),
	coevent(fd),
	service_(service),
	handling_(0) {

	service_->add(this);
}

coservice_item::~coservice_item() {
	assert(getState() == DONE);
}

coservice::coservice() :
	item_count_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128) {

	queue_.put_back(nullptr);
	assert(epfd_ > 0);
}

coservice::~coservice() {
	close(epfd_);
	for (auto ptr : done_items_)
		delete ptr;
}

void coservice::add(coservice_item* cst) {
	epoll_event ev;
	ev.events = cst->getEvents();
	ev.data.ptr = cst;
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
	++item_count_;
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
	--item_count_;
}

size_t coservice::run() {
	thread_local std::vector<coservice_item*> local_done_items;
	size_t count = 0;
	coservice_item* cst;
	int numevents, timeout;
	
	while (item_count_) {
		cst = queue_.take_front();
		if (cst == nullptr) {
			if (queue_.empty())
				timeout = 100;
			else
				timeout = 0;
			numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), timeout);

			if (numevents > 0) {
				for (int i = 0; i < numevents; ++i) {
					cst = (coservice_item*)revents_[i].data.ptr;
					if (cst->handling_)
						continue;

					cst->handling_ = 1;
					cst->setRevents(revents_[i].events);

					queue_.put_back(cst);
				}

				if (static_cast<size_t>(numevents) == revents_.size())
					revents_.resize(revents_.size() * 2);
			}

			for (auto ptr : local_done_items)
				delete ptr;
			local_done_items.clear();

			queue_.put_back(nullptr);
		}
		else {
			coservice_item::running_cst_ = cst;
			cst->resume();
			coservice_item::running_cst_ = nullptr;

			if (cst->getState() == coservice_item::DONE) {
				remove(cst);
				local_done_items.push_back(cst);//这里与coloop的处理方式不同
			}
			else
				cst->handling_ = 0;
		}
		++count;
	}

	{
		klock<kmutex> x(&mutex_);
		for (auto ptr : local_done_items)
			done_items_.push_back(ptr);
	}
	local_done_items.clear();

	return count;
}