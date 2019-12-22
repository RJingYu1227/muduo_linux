#pragma once

#include<pax/base/uncopyable.h>

#include<sys/time.h>
#include<assert.h>

#include<vector>

namespace pax {

template<typename T>
class timewheel;

template<typename T>
class timenode {
	friend class timewheel<T>;
public:

	T& operator*() { return val_; }
	const T& operator*()const { return val_; }

	bool isInlink()const { return (prev_ || next_); }

private:

	void remove();
	void join(timenode* head);

	timenode* prev_ = nullptr;
	T val_;
	timenode* next_ = nullptr;

	timewheel<T>* occupier_ = nullptr;
	size_t rotation_ = 0;

};

template<typename T>
void timenode<T>::remove() {
	if (prev_)
		prev_->next_ = next_;
	if (next_)
		next_->prev_ = prev_;

	prev_ = nullptr;
	next_ = nullptr;
}

template<typename T>
void timenode<T>::join(timenode<T>* head) {
	if (head == nullptr)
		return;

	if (head->next_) {
		next_ = head->next_;
		head->next_->prev_ = this;
	}
	prev_ = head;
	head->next_ = this;
}

//时间精度ms
template<typename T>
class timewheel :uncopyable {
public:

	static uint64_t getMilliSeconds();

	timewheel(size_t wheels);//设置时间轮盘的容量，容量越大，效率越高
	~timewheel() {}

	void addTimenode(uint64_t ms, timenode<T>* node);//相对到期时间
	void removeTimenode(timenode<T>* node);
	size_t getTimeouts(std::vector<timenode<T>*>& vec);

private:

	size_t wheels_;
	size_t node_count_;

	size_t index_;
	uint64_t last_time_;
	std::vector<timenode<T>> time_wheels_;

};

template<typename T>
uint64_t timewheel<T>::getMilliSeconds() {
	timeval now = { 0 };
	gettimeofday(&now, NULL);
	uint64_t u = now.tv_sec;
	u *= 1000;
	u += now.tv_usec / 1000;

	return u;
}

template<typename T>
timewheel<T>::timewheel(size_t wheels) :
	wheels_(wheels),
	node_count_(0),
	index_(0),
	last_time_(0),
	time_wheels_(wheels) {

}

template<typename T>
void timewheel<T>::addTimenode(uint64_t ms, timenode<T>* node) {
	assert(node->occupier_ == nullptr);

	uint64_t expire_time = getMilliSeconds();
	if (node_count_ == 0)//注意这里，既保证了准确性又使getTimeouts可以提前返回
		last_time_ = expire_time;

	if (ms)
		expire_time += ms;
	else
		expire_time += 1;

	size_t diff = expire_time - last_time_;//不可能为0

	node->occupier_ = this;
	node->rotation_ = diff % wheels_ ? diff / wheels_ : diff / wheels_ - 1;
	
	node->join(&time_wheels_[(index_ + diff) % wheels_]);

	++node_count_;
}

template<typename T>
void timewheel<T>::removeTimenode(timenode<T>* node) {
	assert(node->occupier_ == this);

	--node_count_;

	node->remove();
	node->occupier_ = nullptr;
}

template<typename T>
size_t timewheel<T>::getTimeouts(std::vector<timenode<T>*>& vec) {
	size_t num = 0;
	if (node_count_) {
		uint64_t now = getMilliSeconds();
		size_t diff = now - last_time_;
		last_time_ = now;

		timenode<T>* head;
		timenode<T>* node;

		while (diff-- && node_count_) {
			if (++index_ == wheels_)
				index_ = 0;

			head = &time_wheels_[index_];
			while ((node = head->next_)) {
				if (node->rotation_ == 0) {
					removeTimenode(node);

					vec.push_back(node);
					++num;
				}
				else {
					node->rotation_ -= 1;
					head = node;
				}
			}
		}
	}

	return num;
}

}//namespace pax