#include"coservice.h"

#include<assert.h>
#include<unistd.h>

thread_local coservice_item* coservice_item::running_cst_ = nullptr;

coservice_item::coservice_item(int fd, const functor& func, coservice* service) :
	coroutine_item(func),
	coevent(fd),
	service_(service),
	handling_(0) {

	timenode_.setValue(this);
	service_->add(this);
}

coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::move(func)),
	coevent(fd),
	service_(service),
	handling_(0) {

	timenode_.setValue(this);
	service_->add(this);
}

coservice_item::~coservice_item() {
	assert(getState() == DONE);
}

void coservice_item::setTimeout(unsigned int ms) {
	klock<kmutex> x(&service_->time_mutex_);
	service_->timewheel_.setTimeout(ms, &timenode_);
}

coservice::coservice() :
	item_count_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	timewheel_(60 * 1000) {

	cst_queue_.put_back(nullptr);//注意这里
	assert(epfd_ > 0);
}

coservice::~coservice() {
	assert(item_count_ == 0);
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

void coservice::getRevents() {
	coservice_item* cst;
	int numevents;

	if (cst_queue_.empty())
		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
	else
		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 0);
	
	if (numevents > 0) {
		for (int i = 0; i < numevents; ++i) {
			cst = (coservice_item*)revents_[i].data.ptr;

			if (cst->handling_)
				continue;
			{
				klock<kmutex> x(&time_mutex_);
				if (cst->timenode_.isInlink())
					timewheel_.cancelTimeout(&cst->timenode_);
			}
			cst->handling_ = 1;
			cst->setRevents(revents_[i].events);

			cst_queue_.put_back(cst);
		}

		if (static_cast<size_t>(numevents) == revents_.size())
			revents_.resize(revents_.size() * 2);
	}
}

void coservice::getTimeout() {
	{
		klock<kmutex> x(&time_mutex_);
		timewheel_.getTimeout(timenodes_);
	}

	coservice_item* cst;
	for (auto node : timenodes_) {
		cst = node->getValue();

		if (cst->handling_)
			continue;
		cst->handling_ = 1;
		cst->setRevents(0);

		cst_queue_.put_back(cst);
	}
	timenodes_.clear();
}

size_t coservice::run() {
	std::vector<coservice_item*> local_done_items;
	size_t count = 0;
	coservice_item* cst;
	
	while (item_count_) {
		cst = cst_queue_.take_front();
		if (cst == nullptr) {
			//只要handling_ = 1，cst就不会再次进入队列
			getRevents();
			getTimeout();

			for (auto ptr : local_done_items)
				delete ptr;
			local_done_items.clear();
			{
				klock<kmutex> x(&done_items_mutex_);
				for (auto ptr : done_items_)
					delete ptr;
				done_items_.clear();
			}

			cst_queue_.put_back(nullptr);
		}
		else {
			coservice_item::running_cst_ = cst;
			cst->resume();
			coservice_item::running_cst_ = nullptr;

			if (cst->getState() == coservice_item::DONE) {
				remove(cst);
				local_done_items.push_back(cst);
			}
			else
				cst->handling_ = 0;
		}
		++count;
	}

	//在这里直接delete掉local_done_items里的实例，不是线程安全的
	{
		klock<kmutex> x(&done_items_mutex_);
		for (auto ptr : local_done_items)
			done_items_.push_back(ptr);
	}

	return count;
}