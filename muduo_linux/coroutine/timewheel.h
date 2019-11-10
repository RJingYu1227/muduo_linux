#pragma once

#include"uncopyable.h"

#include<vector>
#include<sys/time.h>

template<typename T>
class klinknode :uncopyable {
public:

	klinknode() :
		prev_(nullptr),
		val_(T()),
		next_(nullptr) {

	}

	klinknode* prev() { return prev_; }
	klinknode* next() { return next_; }

	void setValue(const T& val) { val_ = val; }
	void setValue(T&& val) { val_ = std::move(val); }
	T& getValue() { return val_; }

	void remove();
	void join(klinknode* head);
	inline bool isInlink()const;

private:

	klinknode* prev_;
	T val_;
	klinknode* next_;

};

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
void klinknode<T>::join(klinknode<T>* head) {
	if (head == nullptr)
		return;

	if (head->next_) {
		next_ = head->next_;
		head->next_->prev_ = this;
	}
	prev_ = head;
	head->next_ = this;
}

template<typename T>
bool klinknode<T>::isInlink()const {
	if (prev_ || next_)
		return 1;

	return 0;
}

template<typename T>
class timewheel :uncopyable {
public:

	static uint64_t getMilliSeconds();

	timewheel(size_t wheels);
	~timewheel() {}

	void setTimeout(uint64_t ms, klinknode<T>* timenode);
	inline void cancelTimeout(klinknode<T>* timenode);
	size_t getTimeout(std::vector<klinknode<T>*>& vec);

private:

	size_t wheels_;
	size_t count_;

	size_t tindex_;
	uint64_t last_time_;
	std::vector<klinknode<T>> time_wheel_;

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
	count_(0),
	tindex_(0),
	last_time_(0),
	time_wheel_(wheels) {

}

template<typename T>
void timewheel<T>::setTimeout(uint64_t ms, klinknode<T>* timenode) {
	uint64_t expire = getMilliSeconds();
	if (count_ == 0)//注意这里，既保证了准确性又使getTimeout可以提前返回
		last_time_ = expire;
	if (ms == 0)
		ms = 1;
	expire += ms;

	size_t diff = expire - last_time_;
	if (diff > wheels_)
		diff = wheels_;

	timenode->join(&time_wheel_[(tindex_ + diff) % wheels_]);
	++count_;
}

template<typename T>
void timewheel<T>::cancelTimeout(klinknode<T>* timenode) {
	timenode->remove();
	--count_;
}

template<typename T>
size_t timewheel<T>::getTimeout(std::vector<klinknode<T>*>& vec) {
	size_t num = 0;
	if (count_) {
		uint64_t now = getMilliSeconds();
		size_t diff = now - last_time_;
		last_time_ = now;

		klinknode<T>* node;
		while (diff && count_) {
			--diff;
			++tindex_;
			if (tindex_ == wheels_)
				tindex_ = 0;

			while ((node = time_wheel_[tindex_].next())) {
				node->remove();//这里可以优化
				--count_;

				vec.push_back(node);
				++num;
			}
		}
	}

	return num;
}