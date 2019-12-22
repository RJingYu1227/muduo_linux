#pragma once

#include<atomic>

namespace pax {

template<typename T>
class sharedatomic {
public:

	std::atomic<T>& operator*() { return val_; }
	const std::atomic<T>& operator*()const { return val_; }

private:
	enum cache {
		line_size = 64,//intel i5-7500k
		padding_size = line_size - sizeof(std::atomic<T>)
	};

	void staticCheck() { static_assert(sizeof(std::atomic<T>) < 64); }

	char front_[padding_size];

	std::atomic<T> val_;

	char back_[padding_size];

};

}//namespace pax