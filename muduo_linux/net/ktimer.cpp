#include"ktimer.h"

#include<string>

ktimer::ktimer(const functor &cb, uint64_t time, double seconds)
	:func(cb),
	born_(time),
	time_(time),
	interval_(static_cast<uint64_t>(seconds * 1000 * 1000)) {

}

ktimer::ktimer(functor&& cb, uint64_t time, double seconds)
	:func(std::move(cb)),
	born_(time),
	time_(time),
	interval_(static_cast<uint64_t>(seconds * 1000 * 1000)) {

}
