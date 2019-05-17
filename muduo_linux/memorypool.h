#pragma once

#include<queue>
#include<memory>
#include<assert.h>

template<typename T>
class memorypool//这是在堆上申请空间，地址从低到高
{
public:
	memorypool(int init = 64);
	~memorypool();

	void setPtr(T* &ptr);
	void destroyPtr(T* ptr);
	int size() { return size_; }

private:
	void makeSpace();

	int size_;
	std::allocator<T> manager_;

	struct head {
		head() {}
		head(T* ptr_, int size_) {
			this->ptr_ = ptr_;
			this->size_ = size_;
		}
		T* ptr_ = nullptr;
		int size_ = 0;
	};

	std::queue<head> head_queue_;
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
	//析构之后再收回地址
	ptr_queue_.push(ptr);
}

template<typename T>
void memorypool<T>::makeSpace() {
	T* ptr_ = manager_.allocate(size_);
	head head_(ptr_, size_);
	head_queue_.push(head_);

	for (int i = 0; i < size_; ++i) {
		ptr_queue_.push(ptr_);
		ptr_ += 1;
	}
	size_ *= 2;
}