#include"coservice.h"

#include<assert.h>
#include<unistd.h>

void coservice::coservice_item::coroutineFunc(coservice_item* cst) {
	cst->Func();

	cst->tie_->done_ = 1;//注意这里
	cst->service_->remove(cst);
}

coservice::coservice_item::coservice_item(int fd, const functor& func, coservice* service) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	service_(service),
	tie_(new impl(this)),
	Func(func) {

	service_->add(this);
}

coservice::coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	service_(service),
	tie_(new impl(this)),
	Func(std::move(func)) {

	service_->add(this);
}

coservice::coservice_item::~coservice_item() {
	if (getState() != coroutine_item::DONE)
		service_->remove(this);
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
}

void coservice::add(coservice_item* cst) {
	epoll_event ev;
	ev.events = cst->getEvents();
	ev.data.ptr = cst->tie_;
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
	++item_count_;
}

void coservice::modify(coservice_item* cst) {
	epoll_event ev;
	ev.events = cst->getEvents();
	ev.data.ptr = cst->tie_;
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

void coservice::remove(coservice_item* cst) {
	int fd = cst->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
	--item_count_;
}

size_t coservice::run() {
	thread_local std::vector<impl*> done_ties;

	size_t count = 0;
	impl* tie;
	coservice_item* cst;
	int numevents, timeout;
	
	while (item_count_) {
		tie = queue_.take_front();
		if (tie == nullptr) {
			if (queue_.empty())
				timeout = 100;
			else
				timeout = 0;
			numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), timeout);

			if (numevents > 0) {
				for (int i = 0; i < numevents; ++i) {
					tie = (impl*)revents_[i].data.ptr;
					if (tie->inqueue_)
						continue;

					tie->inqueue_ = 1;
					cst = (coservice_item*)tie->ptr_;
					cst->setRevents(revents_[i].events);

					queue_.put_back(tie);
				}

				if (static_cast<size_t>(numevents) == revents_.size())
					revents_.resize(revents_.size() * 2);
			}
			queue_.put_back(nullptr);

			for (auto ptr : done_ties)
				delete ptr;
			done_ties.clear();
		}
		else {
			cst = (coservice_item*)tie->ptr_;
			cst->resume();

			if (tie->done_)
				done_ties.push_back(tie);
			else
				tie->inqueue_ = 0;
		}
		++count;
	}

	return count;
}