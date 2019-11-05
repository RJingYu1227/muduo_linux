#include"coloop.h"

#include<assert.h>
#include<unistd.h>
#include<sys/time.h>

kthreadlocal<coloop> coloop::thread_loop_(coloop::freeColoop);

coloop::coloop_item::coloop_item(coroutine_t id, int fd) :
	loop_(threadColoop()),
	id_(id),
	fd_(fd),
	events_(0),
	revents_(0),
	timeout_({ 0,this,0 }) {

	loop_->add(this);
}

coloop::coloop_item::~coloop_item() {

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

void coloop::setTimeout(unsigned int ms, coloop_item::timeout_t* timeout) {
	if (ms == 0)
		ms = 1;
	uint64_t expire = getMilliSeconds() + ms;

	int diff = static_cast<int>(expire - last_time_);
	if (diff > 59999)
		diff = 59999;

	coloop_item::timeout_t* head = &time_wheel_[(tindex_ + diff) % 60000];
	if (head->next_) {
		timeout->next_ = head->next_;
		head->next_->prev_ = timeout;
	}
	timeout->prev_ = head;
	head->next_ = timeout;
	
}

void coloop::loopFunc() {
	while (!quit_) {
		int numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		if (numevents > 0) {
			for (int i = 0; i < numevents; ++i) {
				coloop_item* cpt = (coloop_item*)revents_[i].data.ptr;
				cpt->revents_ = revents_[i].events;
				if (cpt->timeout_.prev_) {
					cpt->timeout_.prev_->next_ = cpt->timeout_.next_;
					if (cpt->timeout_.next_)
						cpt->timeout_.next_->prev_ = cpt->timeout_.prev_;
					
					cpt->timeout_.prev_ = nullptr;
					cpt->timeout_.next_ = nullptr;
				}

				coroutine::resume(cpt->id_);
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

			coloop_item::timeout_t* node = time_wheel_[tindex_].next_;
			while (node) {
				node->prev_->next_ = nullptr;
				node->prev_ = nullptr;

				node->cpt_->revents_ = 0;
				coroutine::resume(node->cpt_->id_);

				node = node->next_;
			}
		}
	}
}