#include"coloop.h"

#include<assert.h>
#include<unistd.h>

void coloop::coloop_item::coroutineFunc(coloop_item* cpt) {
	cpt->Func();

	cpt->loop_->remove(cpt);
}

coloop::coloop_item::coloop_item(int fd, const functor& func, coloop* loop) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	loop_(loop),
	Func(func) {

	timenode_.setValue(this);
	loop_->add(this);
}

coloop::coloop_item::coloop_item(int fd, functor&& func, coloop* loop) :
	coroutine_item(std::bind(coroutineFunc, this)),
	coevent(fd),
	loop_(loop),
	Func(std::move(func)) {

	timenode_.setValue(this);
	loop_->add(this);
}

coloop::coloop_item::~coloop_item() {
	if (getState() == coroutine_item::FREE)
		loop_->remove(this);
}

void coloop::coloop_item::setTimeout(unsigned int ms) {
	assert(getState() == coroutine_item::RUNNING);
	loop_->time_wheel_.setTimeout(ms, &timenode_);

	coroutine::yield();//注意这里
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

void coloop::loop() {
	assert(!looping_);
	looping_ = 1;
	quit_ = 0;

	std::vector<klinknode<coloop_item*>*> timenodes;
	while (!quit_) {
		int numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				coloop_item* cpt = (coloop_item*)revents_[i].data.ptr;
				//这里有线程安全问题，cpt可能未执行就已经被析构
				if (cpt->timenode_.isInlink())
					cpt->loop_->time_wheel_.cancelTimeout(&cpt->timenode_);

				cpt->setRevents(revents_[i].events);
				cpt->resume();
			}

			if (static_cast<size_t>(numevents) == revents_.size())
				revents_.resize(revents_.size() * 2);
		}

		if (time_wheel_.getTimeout(timenodes)) {
			for (auto node : timenodes) {
				node->getValue()->setRevents(0);
				node->getValue()->resume();
			}

			timenodes.clear();
		}
	}

	looping_ = 0;
}