#pragma once

#include<queue>
#include<memory>
#include<assert.h>

template<typename T>
struct head {
	head() {}
	head(T* ptr_, int size_) {
		this->ptr_ = ptr_;
		this->size_ = size_;
	}
	T* ptr_ = nullptr;
	int size_ = 0;
};

template<typename T>
class memorypool {
public:
	memorypool(int init = 1024);
	~memorypool();

	void setPtr(T* &ptr);
	void destroyPtr(T* ptr);
	int size() { return size_; }

private:
	void makeSpace();

	int size_;
	std::allocator<T> manager_;
	std::queue<head<T>> head_queue_;
	std::queue<T*> ptr_queue_;
};

template<typename T>
memorypool<T>::memorypool(int init) {
	size_ = init;
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

	for (int i = 0; i < size_; ++i) {
		ptr_queue_.push(ptr_);
		ptr_ += 1;
	}
	size_ *= 2;
}