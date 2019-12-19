#include"poller.h"
#include"epoll.h"
#include"poll.h"

using namespace pax;

poller* poller::newPoller(eventloop* loop, poller::POLLER p) {
	if (p == kEPOLL)
		return new epoll(loop);
	else if (p == kPOLL)
		return new poll(loop);
	else
		return nullptr;
}
