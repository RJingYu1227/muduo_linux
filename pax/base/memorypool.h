﻿#pragma once

#include<pax/base/uncopyable.h>

#include<queue>
#include<memory>

namespace pax {

template<typename T>
struct head {
	head() {}
	head(T* ptr, size_t size)
		:ptr_(ptr),
		size_(size) {

	}
	T* ptr_ = nullptr;
	size_t size_ = 0;
};

template<typename T>
class memorypool :uncopyable {
public:
	memorypool(size_t init = 128);
	~memorypool();

	void setPtr(T* &ptr);
	void destroyPtr(T* ptr);
	size_t size() { return size_; }

private:
	void makeSpace();

	size_t size_;
	std::allocator<T> manager_;
	std::queue<head<T>> head_queue_;
	std::queue<T*> ptr_queue_;
};

template<typename T>
memorypool<T>::memorypool(size_t init)
	:size_(init) {

	makeSpace();
	size_ /= 2;
}

template<typename T>
memorypool<T>::~memorypool() {
	head<T> temp_;
	while (!head_queue_.empty()) {
		temp_ = head_queue_.front();
		head_queue_.pop();
		manager_.deallocate(temp_.ptr_, temp_.size_);
	}
}

template<typename T>
void memorypool<T>::setPtr(T* &ptr) {
	if (ptr_queue_.empty())
		makeSpace();
	ptr = ptr_queue_.front();
	ptr_queue_.pop();
}

template<typename T>
void memorypool<T>::destroyPtr(T* ptr) {
	manager_.destroy(ptr);
	ptr_queue_.push(ptr);
}

template<typename T>
void memorypool<T>::makeSpace() {
	T* ptr_ = manager_.allocate(size_);
	head<T> head_(ptr_, size_);
	head_queue_.push(head_);

	for (size_t i = 0; i < size_; ++i) {
		ptr_queue_.push(ptr_);
		ptr_ += 1;
	}
	size_ *= 2;
}

}//namespace pax