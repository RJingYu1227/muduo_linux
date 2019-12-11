#include"coservice.h"

#include<assert.h>
#include<unistd.h>

thread_local coservice_item* coservice_item::running_cst_ = nullptr;

coservice_item::coservice_item(int fd, const functor& func, coservice* service) :
	coroutine_item(func),
	coevent(fd),
	service_(service),
	handling_(1) {

	timenode_.setValue(this);
	setRevents(0);
	//另外一种启动方式
	service_->add(this);
	service_->cst_queue_.put(this);
}

coservice_item::coservice_item(int fd, functor&& func, coservice* service) :
	coroutine_item(std::move(func)),
	coevent(fd),
	service_(service),
	handling_(1) {

	timenode_.setValue(this);
	setRevents(0);

	service_->add(this);
	service_->cst_queue_.put(this);
}

coservice_item::~coservice_item() {
	assert(getState() == DONE);
}

void coservice::yield(unsigned int ms) {
	coservice_item* cst = coservice_item::running_cst_;
	assert(cst != nullptr);
	cst->service_->setTimeout(ms, &cst->timenode_);

	coroutine::yield();
}

coservice::coservice() :
	item_count_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	cst_queue_(2048),
	timewheel_(60 * 1000) {

	cst_queue_.put(nullptr);//注意这里
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

void coservice::doItem(coservice_item* cst) {
	coservice_item::running_cst_ = cst;
	cst->resume();
	coservice_item::running_cst_ = nullptr;

	if (cst->getState() == coservice_item::DONE) {
		remove(cst);

		klock<kmutex> lock(&done_items_mutex_);
		done_items_.push_back(cst);
	}
	else
		cst->handling_ = 0;
}

void coservice::getItems() {
	int numevents;
	coservice_item* cst;

	{
		klock<kmutex> x(&time_mutex_);

		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		//此时cst_queue_必定是空的，最多只有和线程数相当的任务在执行，如果妹有立即返回，说明其实并不忙
		//这么做是为了正确处理拿到锁之前设置好的超时事件
		//这一次处理完毕之后的超时事件也是可以正确处理的
		for (int i = 0; i < numevents; ++i) {
			cst = (coservice_item*)revents_[i].data.ptr;
			if (cst->timenode_.isInlink())
				timewheel_.cancelTimeout(&cst->timenode_);

			if (cst->handling_)
				revents_[i].data.ptr = nullptr;
			//不能使会导致重复resume的事情发生
			//且考虑到setTimeout在这之后才拿到锁，会导致超时并未正确处理，所以这一步不可以在外面操作
		}

		timewheel_.getTimeout(timenodes_);
		for (size_t i = 0; i < timenodes_.size(); ++i) {
			cst = timenodes_[i]->getValue();
			if (cst->handling_) {
				timewheel_.setTimeout(1, &cst->timenode_);
				timenodes_[i] = nullptr;
			}
		}
		//协程在设置了超时后，并没有及时的将handling_置为0，这是有可能的，其实发生重复resume的几率也很小
		//妹办法，为了线程安全
		//这一步其实可以在外面操作，为了易于理解和代码结构的优雅，我把它放在了这里
	}

	for (int i = 0; i < numevents; ++i) {
		cst = (coservice_item*)revents_[i].data.ptr;
		if (cst == nullptr)
			continue;

		cst->handling_ = 1;
		cst->setRevents(revents_[i].events);
		cst_queue_.put(cst);
	}

	for (auto node : timenodes_) {
		cst = node->getValue();
		if (cst == nullptr)
			continue;

		cst->handling_ = 1;
		cst->setRevents(0);
		cst_queue_.put(cst);
	}

	if (numevents > 0 && static_cast<size_t>(numevents) == revents_.size())
		revents_.resize(revents_.size() * 2);
	timenodes_.clear();
}

void coservice::setTimeout(unsigned int ms, klinknode<coservice_item*>* timenode) {
	klock<kmutex> x(&time_mutex_);
	//if (timenode->isInlink())
	//	timewheel_.cancelTimeout(timenode);
	timewheel_.setTimeout(ms, timenode);
}

size_t coservice::run() {
	//可以为每一个执行run的线程提供一个私有的vector
	//因为只有一个值为nullptr的cst
	//std::vector<coservice_item*> local_done_items;
	size_t count = 0;
	coservice_item* cst;
	
	while (item_count_) {
		cst_queue_.take(cst);
		if (cst == nullptr) {
			getItems();
			//只要handling_ = 1，cst就不会再次进入队列
			//且此时的事件绝大多数情况下是妹有处理过的
			//极少数情况下，在epoll_wait返回和判断handing_之间给处理了，超时除外
			//要优化的代价很大

			{
				klock<kmutex> lock(&done_items_mutex_);
				for (auto ptr : done_items_)
					delete ptr;
				done_items_.clear();
			}

			cst_queue_.put(nullptr);
		}
		else
			doItem(cst);

		++count;
	}

	return count;
}