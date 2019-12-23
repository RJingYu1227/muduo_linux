#include<pax/coroutine/coloop.h>

#include<assert.h>
#include<unistd.h>

using namespace pax;

thread_local coloop_item* coloop_item::running_cpt_ = nullptr;

coloop_item::coloop_item(const functor& func, int fd, sockaddr_in& addr, coloop* loop) :
	coroutine_item(func, 0),
	coevent(fd, addr),
	loop_(loop) {

	*timenode_ = this;

	loop_->setTimeout(1, &timenode_);
}

coloop_item::coloop_item(const functor& func, coloop* loop) :
	coroutine_item(func, 0),
	loop_(loop) {

	assert(socket::valid());

	*timenode_ = this;

	loop_->setTimeout(1, &timenode_);
}

coloop_item::coloop_item(functor&& func, int fd, sockaddr_in& addr, coloop* loop) :
	coroutine_item(std::move(func), 0),
	coevent(fd, addr),
	loop_(loop) {

	*timenode_ = this;

	loop_->setTimeout(1, &timenode_);
}

coloop_item::coloop_item(functor&& func, coloop* loop) :
	coroutine_item(std::move(func), 0),
	loop_(loop) {

	assert(socket::valid());

	*timenode_ = this;

	loop_->setTimeout(1, &timenode_);
}

coloop_item::~coloop_item() {
	assert(getState() == DONE);
}

void coloop_item::yield(double seconds) {
	assert(this == running_cpt_);
	assert(seconds > 0);

	uint64_t ms = static_cast<uint64_t>(seconds * 1000);
	loop_->setTimeout(ms, &timenode_);

	coroutine::yield();
}

void coloop_item::yield(int ms) {
	assert(this == running_cpt_);
	if (ms >= 0)
		loop_->setTimeout(ms, &timenode_);

	coroutine::yield();
}

coloop::coloop() :
	looping_(0),
	quit_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	timewheel_(60 * 1000) {

	assert(epfd_ > 0);
}

coloop::~coloop() {
	assert(!looping_);
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
	if (cpt->getState() == coloop_item::FREE)
		add(cpt);

	coloop_item::running_cpt_ = cpt;
	cpt->resume();
	coloop_item::running_cpt_ = nullptr;

	if (cpt->getState() == coloop_item::DONE) {
		remove(cpt);
		delete cpt;
	}
}

void coloop::setTimeout(uint64_t ms, timenode<coloop_item*>* timenode) {
	lock<mutex> lock(&time_mutex_);
	timewheel_.addTimenode(ms, timenode);
}

void coloop::loop() {
	assert(!looping_);
	looping_ = 1;
	quit_ = 0;

	coloop_item* cpt;
	int numevents;

	while (!quit_) {
		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);

		{
			lock<mutex> x(&time_mutex_);

			for (int i = 0; i < numevents; ++i) {
				cpt = (coloop_item*)revents_[i].data.ptr;
				if (cpt->timenode_.isInlink())
					timewheel_.removeTimenode(&cpt->timenode_);
			}

			timewheel_.getTimeouts(timenodes_);
		}

		for (int i = 0; i < numevents; ++i) {
			cpt = (coloop_item*)revents_[i].data.ptr;
			cpt->setRevents(revents_[i].events);
			
			doItem(cpt);
		}

		for (auto node : timenodes_) {
			cpt = **node;
			cpt->setRevents(0);

			doItem(cpt);
		}

		if (numevents > 0 && static_cast<size_t>(numevents) == revents_.size())
			revents_.resize(revents_.size() * 2);
		timenodes_.clear();
	}

	looping_ = 0;
}