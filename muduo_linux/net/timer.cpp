#include"timer.h"

#include<string>

using namespace pax;

timer::timer(const functor &cb, uint64_t time, double seconds)
	:func(cb),
	born_(time),
	time_(time),
	interval_(static_cast<uint64_t>(seconds * 1000 * 1000)) {

}

timer::timer(functor&& cb, uint64_t time, double seconds)
	:func(std::move(cb)),
	born_(time),
	time_(time),
	interval_(static_cast<uint64_t>(seconds * 1000 * 1000)) {

}
