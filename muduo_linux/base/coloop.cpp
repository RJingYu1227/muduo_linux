#include "coloop.h"

#include<assert.h>
#include<unistd.h>

const int coloop::kNoneEvent = 0;
const int coloop::kReadEvent = EPOLLIN | EPOLLPRI;
const int coloop::kWriteEvent = EPOLLOUT;
const int coloop::kEpollet = EPOLLET;

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
}

coloop::coloop()
	:quit_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128) {

	assert(epfd_ > 0);
}

coloop::~coloop() {
	close(epfd_);
}

int coloop::add(coloop_t item) {
	epoll_event ev;
	ev.events = item.events_;
	ev.data.u32 = item.coid_;
	
	return epoll_ctl(epfd_, EPOLL_CTL_ADD, item.fd_, &ev);
}

int coloop::update(coloop_t item) {
	epoll_event ev;
	ev.events = item.events_;
	ev.data.u32 = item.coid_;

	return epoll_ctl(epfd_, EPOLL_CTL_MOD, item.fd_, &ev);
}

int coloop::remove(coloop_t item) {
	epoll_event ev;
	ev.events = item.events_;
	ev.data.u32 = item.coid_;

	return epoll_ctl(epfd_, EPOLL_CTL_DEL, item.fd_, &ev);
}

void coloop::loop(int timeoutms) {
	coroutine* coenv = coroutine::threadCoenv();
	coroutine_t id;
	while (!quit_) {
		int numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), timeoutms);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				id = revents_[i].data.u32;
				coenv->resume(id);
			}

			if (static_cast<size_t>(numevents) == revents_.size())
				revents_.resize(revents_.size() * 2);
		}
	}
}