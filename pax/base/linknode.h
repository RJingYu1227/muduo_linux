#pragma once

namespace pax {

template<typename T>
class linknode {
public:

	linknode* prev() { return prev_; }
	linknode* next() { return next_; }

	T& operator*() { return val_; }
	const T& operator*()const { return val_; }

	void remove();
	void join(linknode* head);
	bool isInlink()const { return (prev_ || next_); }

private:

	linknode* prev_ = nullptr;
	T val_;
	linknode* next_ = nullptr;

};

template<typename T>
void linknode<T>::remove() {
	if (prev_)
		prev_->next_ = next_;
	if (next_)
		next_->prev_ = prev_;

	prev_ = nullptr;
	next_ = nullptr;
}

template<typename T>
void linknode<T>::join(linknode<T>* head) {
	if (head == nullptr)
		return;

	if (head->next_) {
		next_ = head->next_;
		head->next_->prev_ = this;
	}
	prev_ = head;
	head->next_ = this;
}

}//namespace pax