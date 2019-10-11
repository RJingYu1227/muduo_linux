#include"coloop.h"

#include<assert.h>
#include<unistd.h>
#include<sys/time.h>

kthreadlocal<coloop> coloop::thread_loop_;

coloop* coloop::threadColoop() {
	coloop* cp = thread_loop_.get();
	if (cp == nullptr) {
		cp = new coloop();
		thread_loop_.set(cp);
	}

	return cp;
}

void coloop::freeColoop() {
	coloop* cp = thread_loop_.get();
	assert(cp != nullptr);
	delete cp;
	thread_loop_.set(nullptr);
}

coloop::coloop()
	:quit_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	tindex_(0),
	last_time_(getMilliSeconds()),
	time_wheel_(60 * 1000) {

	assert(epfd_ > 0);
}

coloop::~coloop() {
	close(epfd_);
}

int coloop::add(int fd, int ce, coroutine_t id) {
	epoll_event ev;
	ev.events = ce;
	ev.data.u32 = id;
	
	return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

int coloop::modify(int fd, int ce, coroutine_t id) {
	epoll_event ev;
	ev.events = ce;
	ev.data.u32 = id;

	return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

int coloop::remove(int fd) {
	return epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
}

uint64_t coloop::getMilliSeconds() {
	timeval now = { 0 };
	gettimeofday(&now, NULL);
	uint64_t u = now.tv_sec;
	u *= 1000;
	u += now.tv_usec / 1000;
	return u;
}

void coloop::runAfter(unsigned int ms, coroutine_t id) {
	uint64_t now = getMilliSeconds();
	if (ms == 0)
		ms = 1;
	now += ms;

	int diff = static_cast<int>(now - last_time_);
	if (diff > 59999)
		diff = 59999;

	time_wheel_[(tindex_ + diff) % 60000].push_back(id);
}

void coloop::loop() {
	coroutine* coenv = coroutine::threadCoenv();
	coroutine_t id;
	while (!quit_) {
		int numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				id = revents_[i].data.u32;
				coenv->resume(id);
			}

			if (static_cast<size_t>(numevents) == revents_.size())
				revents_.resize(revents_.size() * 2);
		}

		uint64_t now = getMilliSeconds();
		int diff = static_cast<int>(now - last_time_);
		last_time_ = now;

		while (diff--) {
			++tindex_;
			if (tindex_ == 60000)
				tindex_ = 0;

			for (auto id : time_wheel_[tindex_])
				coenv->resume(id);
			time_wheel_[tindex_].clear();
		}
	}
}