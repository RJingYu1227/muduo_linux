#include<pax/coroutine/coservice.h>

#include<assert.h>
#include<unistd.h>

using namespace pax;

thread_local coservice_item* coservice_item::running_cst_ = nullptr;

coservice_item::coservice_item(const functor& func, int fd, sockaddr_in& addr, coservice* service) :
	coroutine_item(func, 0),
	coevent(fd, addr),
	service_(service) {

	*handling_ = true;
	*timenode_ = this;
	//另外一种启动方式
	service_->add(this);
}

coservice_item::coservice_item(const functor& func, coservice* service) :
	coroutine_item(func, 0),
	service_(service) {

	assert(socket::valid());

	*handling_ = true;
	*timenode_ = this;

	service_->add(this);
}

coservice_item::coservice_item(functor&& func, int fd, sockaddr_in& addr, coservice* service) :
	coroutine_item(std::move(func), 0),
	coevent(fd, addr),
	service_(service) {

	*handling_ = true;
	*timenode_ = this;
	//另外一种启动方式
	service_->add(this);
}

coservice_item::coservice_item(functor&& func, coservice* service) :
	coroutine_item(std::move(func), 0),
	service_(service) {

	assert(socket::valid());

	*handling_ = true;
	*timenode_ = this;

	service_->add(this);
}

coservice_item::~coservice_item() {
	assert(getState() == DONE);
}

void coservice_item::yield(double seconds) {
	assert(this == running_cst_);
	assert(seconds > 0);

	uint64_t ms = static_cast<uint64_t>(seconds * 1000);
	service_->setTimeout(ms, &timenode_);

	coroutine::yield();
}

void coservice_item::yield(int ms) {
	assert(this == running_cst_);
	if (ms >= 0)
		service_->setTimeout(ms, &timenode_);

	coroutine::yield();
}

coservice::coservice() :
	item_count_(0),
	epfd_(epoll_create1(EPOLL_CLOEXEC)),
	revents_(128),
	timewheel_(60 * 1000) {

	task_queue_.push(nullptr);//注意这里
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

	lock<mutex> lock(&task_mutex_);
	task_queue_.push(cst);
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

/*
只要handling_ = 1，cst就不会再次进入队列
且此时的事件绝大多数情况下是妹有处理过的
极少数情况下，在epoll_wait返回和判断handing_之间给处理了，超时除外
要优化的代价很大
*/
void coservice::doReactor() {
	int numevents;
	coservice_item* cst;

	{
		lock<mutex> lock(&time_mutex_);

		numevents = epoll_wait(epfd_, &*revents_.begin(), static_cast<int>(revents_.size()), 1);
		//此时cst_queue_必定是空的，最多只有和线程数相当的任务在执行，如果妹有立即返回，说明其实并不忙
		//这么做是为了正确处理拿到锁之前设置好的超时事件
		//这一次处理完毕之后的超时事件也是可以正确处理的
		for (int i = 0; i < numevents; ++i) {
			cst = (coservice_item*)revents_[i].data.ptr;
			if (cst->timenode_.isInlink())
				timewheel_.removeTimenode(&cst->timenode_);

			if (*cst->handling_ == true)
				revents_[i].data.ptr = nullptr;
			//不能使会导致重复resume的事情发生
			//且考虑到setTimeout在这之后才拿到锁，会导致超时并未正确处理，所以这一步不可以在外面操作
		}

		timewheel_.getTimeouts(timenodes_);

		for (size_t i = 0; i < timenodes_.size(); ++i) {
			cst = **timenodes_[i];
			if (*cst->handling_ == true) {
				timewheel_.addTimenode(1, &cst->timenode_);
				timenodes_[i] = nullptr;
			}
		}
		//协程在设置了超时后，并没有及时的将handling_置为0，这是有可能的，其实发生重复resume的几率也很小
		//妹办法，为了线程安全
		//这一步其实可以在外面操作，为了易于理解和代码结构的优雅，我把它放在了这里
	}

	{
		lock<mutex> lock(&task_mutex_);

		for (int i = 0; i < numevents; ++i) {
			cst = (coservice_item*)revents_[i].data.ptr;
			if (cst == nullptr)
				continue;

			*cst->handling_ = true;
			cst->setRevents(revents_[i].events);
			task_queue_.push(cst);
		}

		for (auto node : timenodes_) {
			if (node == nullptr)
				continue;

			cst = **node;
			*cst->handling_ = true;
			cst->setRevents(0);
			task_queue_.push(cst);
		}

		task_cond_.notifyAll();
	}
	
	{
		lock<mutex> lock(&done_mutex_);
		for (auto ptr : done_items_)
			delete ptr;
		done_items_.clear();
	}

	if (numevents > 0 && static_cast<size_t>(numevents) == revents_.size())
		revents_.resize(revents_.size() * 2);
	timenodes_.clear();
}

void coservice::setTimeout(uint64_t ms, timenode<coservice_item*>* timenode) {
	lock<mutex> lock(&time_mutex_);
	timewheel_.addTimenode(ms, timenode);
}

size_t coservice::run() {
	//可以为每一个执行run的线程提供一个私有的vector
	//因为只有一个值为nullptr的cst
	//std::vector<coservice_item*> local_done_items;
	size_t count = 0;
	coservice_item* cst;
	
	while (item_count_) {
		{
			lock<mutex> lock(&task_mutex_);
			if (task_queue_.empty()) {
				task_cond_.wait(&task_mutex_);
				continue;
			}

			cst = task_queue_.front();
			task_queue_.pop();
		}

		if (cst == nullptr) {
			doReactor();

			lock<mutex> lock(&task_mutex_);
			task_queue_.push(nullptr);
		}
		else {
			coservice_item::running_cst_ = cst;
			cst->resume();
			coservice_item::running_cst_ = nullptr;

			if (cst->getState() == coservice_item::DONE) {
				remove(cst);

				lock<mutex> lock(&done_mutex_);
				done_items_.push_back(cst);
			}
			else
				*cst->handling_ = false;
		}

		++count;
	}

	return count;
}