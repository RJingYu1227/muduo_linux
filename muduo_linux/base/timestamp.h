﻿#pragma once

#include<string>

class timestamp {
public:

	static uint64_t getMicroSeconds();
	static uint64_t getMillSeconds();
	static time_t getSeconds() { return time(nullptr); }

	timestamp() :
		micro_seconds_(0) {

	}

	explicit timestamp(uint64_t us) :
		micro_seconds_(us) {

	}

	void swap(timestamp& that) { std::swap(micro_seconds_, that.micro_seconds_); }

	std::string toFormattedString(bool us = true)const;

	uint64_t microSeconds()const { return micro_seconds_; }
	uint64_t millSeconds()const { return micro_seconds_ / 1000; }
	time_t seconds()const { return static_cast<time_t>(micro_seconds_ / 1000000); }

private:

	uint64_t micro_seconds_;

};

namespace std {
	template<>
	inline void swap<timestamp>(timestamp& lhs, timestamp& rhs) {
		lhs.swap(rhs);
	}
}