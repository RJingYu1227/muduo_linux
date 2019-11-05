#include"coloop.h"

#include<assert.h>
#include<unistd.h>
#include<sys/time.h>

kthreadlocal<coloop> coloop::thread_loop_(coloop::freeColoop);

template<typename T>
void klinknode<T>::remove() {
	if (prev_)
		prev_->next_ = next_;
	if (next_)
		next_->prev_ = prev_;

	prev_ = nullptr;
	next_ = nullptr;
}

template<typename T>
void klinknode<T>::join(klinknode<T>* phead) {
	if (phead == nullptr)
		return;

	if (phead->next_) {
		next_ = phead->next_;
		phead->next_->prev_ = this;
	}
	prev_ = phead;
	phead->next_ = this;
}

void coloop::coloop_item::coroutineFunc(coloop_item* cpt) {
	cpt->Func();

	cpt->cancelTimeout();
	cpt->loop_->remove(cpt);
}

coloop::coloop_item::coloop_item(int fd, const functor& func) :
	coroutine_item(std::bind(coroutineFunc, this)),
	loop_(threadColoop()),
	fd_(fd),
	events_(0),
	revents_(0),
	timeout_({ 0,0,0 }),
	Func(func) {

	loop_->add(this);
}

coloop::coloop_item::coloop_item(int fd, functor&& func) :
	coroutine_item(std::bind(coroutineFunc, this)),
	loop_(threadColoop()),
	fd_(fd),
	events_(0),
	revents_(0),
	timeout_({ 0,0,0 }),
	Func(std::move(func)) {

	loop_->add(this);
}

coloop::coloop_item::~coloop_item() {
	cancelTimeout();
	loop_->remove(this);
}

coloop* coloop::threadColoop() {
	coloop* cp = thread_loop_.get();
	if (cp == nullptr) {
		cp = new coloop();
		thread_loop_.set(cp);
	}

	return cp;
}

void coloop::freeColoop(void* ptr) {
	coloop* cp = (coloop*)ptr;
	delete cp;
}

coloop::coloop()
	:running_(0),
	quit_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	tindex_(0),
	count_(0),
	time_wheel_(60 * 1000) {

	assert(epfd_ > 0);
}

coloop::~coloop() {
	close(epfd_);
}

void coloop::add(coloop_item* cpt) {
	epoll_event ev;
	ev.events = cpt->events_;
	ev.data.ptr = cpt;
	int fd = cpt->fd_;

	epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
}

void coloop::modify(coloop_item* cpt) {
	epoll_event ev;
	ev.events = cpt->events_;
	ev.data.ptr = cpt;
	int fd = cpt->fd_;

	epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
}

void coloop::remove(coloop_item* cpt) {
	int fd = cpt->fd_;

	epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
}

uint64_t coloop::getMilliSeconds() {
	timeval now = { 0 };
	gettimeofday(&now, NULL);
	uint64_t u = now.tv_sec;
	u *= 1000;
	u += now.tv_usec / 1000;
	return u;
}

void coloop::setTimeout(unsigned int ms, klinknode<coloop_item*>* timeout) {
	uint64_t expire = getMilliSeconds();
	if (count_ == 0)
		last_time_ = expire;

	if (ms == 0)
		ms = 1;
	expire += ms;

	int diff = static_cast<int>(expire - last_time_);
	if (diff > 59999)
		diff = 59999;

	timeout->join(&time_wheel_[(tindex_ + diff) % 60000]);
	++count_;
}

void coloop::cancelTimeout(klinknode<coloop_item*>* timeout) {
	if (timeout->val_ != nullptr) {
		timeout->remove();
		timeout->val_ = nullptr;
		--count_;
	}
}

void coloop::loopFunc() {
	assert(!running_);
	running_ = 1;

	while (!quit_) {
		int numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				coloop_item* cpt = (coloop_item*)revents_[i].data.ptr;

				cpt->cancelTimeout();

				cpt->revents_ = revents_[i].events;
				cpt->resume();
			}

			if (static_cast<size_t>(numevents) == revents_.size())
				revents_.resize(revents_.size() * 2);
		}

		if (count_) {
			uint64_t now = getMilliSeconds();
			int diff = static_cast<int>(now - last_time_);
			last_time_ = now;

			klinknode<coloop_item*>* node;
			coloop_item* cpt;
			while (diff && count_) {
				--diff;
				++tindex_;
				if (tindex_ == 60000)
					tindex_ = 0;

				while ((node = time_wheel_[tindex_].next_)) {
					cpt = node->val_;

					node->remove();
					node->val_ = nullptr;
					--count_;

					cpt->revents_ = 0;
					cpt->resume();
				}
			}
		}
	}

	running_ = 0;
}