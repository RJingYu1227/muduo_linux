#include"coloop.h"

#include<assert.h>
#include<unistd.h>

thread_local coloop_item* coloop_item::running_cpt_ = nullptr;

coloop_item::coloop_item(int fd, const functor& func, coloop* loop) :
	coroutine_item(func),
	coevent(fd),
	loop_(loop),
	Func(func) {

	timenode_.setValue(this);
	loop_->add(this);
}

coloop_item::coloop_item(int fd, functor&& func, coloop* loop) :
	coroutine_item(std::move(func)),
	coevent(fd),
	loop_(loop),
	Func(std::move(func)) {

	timenode_.setValue(this);
	loop_->add(this);
}

coloop_item::~coloop_item() {
	assert(getState() == DONE);
}

void coloop::epollTimeout(unsigned int ms) {
	coloop_item* cpt = coloop_item::running_cpt_;
	assert(cpt != nullptr);
	cpt->loop_->time_wheel_.setTimeout(ms, &cpt->timenode_);

	coroutine::yield();
}

coloop::coloop() :
	looping_(0),
	quit_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	time_wheel_(60 * 1000) {

	assert(epfd_ > 0);
}

coloop::~coloop() {
	close(epfd_);
}

void coloop::add(coloop_item* cpt) {
	epoll_event ev;
	ev.events = cpt->getEvents();
	ev.data.ptr = cpt;
	int fd = cpt->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void coloop::modify(coloop_item* cpt) {
	epoll_event ev;
	ev.events = cpt->getEvents();
	ev.data.ptr = cpt;
	int fd = cpt->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

void coloop::remove(coloop_item* cpt) {
	int fd = cpt->getFd();

	epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
}

void coloop::doItem(coloop_item* cpt) {
	coloop_item::running_cpt_ = cpt;
	cpt->resume();
	coloop_item::running_cpt_ = nullptr;
	if (cpt->getState() == coloop_item::DONE) {
		remove(cpt);
		delete cpt;
	}
}

void coloop::loop() {
	assert(!looping_);
	looping_ = 1;
	quit_ = 0;

	coloop_item* cpt;
	int numevents;

	while (!quit_) {
		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				cpt = (coloop_item*)revents_[i].data.ptr;
				if (cpt->timenode_.isInlink())
					cpt->loop_->time_wheel_.cancelTimeout(&cpt->timenode_);

				cpt->setRevents(revents_[i].events);
				doItem(cpt);
			}

			if (static_cast<size_t>(numevents) == revents_.size())
				revents_.resize(revents_.size() * 2);
		}

		if (time_wheel_.getTimeout(timenodes_)) {
			for (auto node : timenodes_) {
				cpt = node->getValue();

				cpt->setRevents(0);
				doItem(cpt);
			}

			timenodes_.clear();
		}
	}

	looping_ = 0;
}