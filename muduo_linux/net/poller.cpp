#include"poller.h"
#include"kepoll.h"

poller* poller::newPoller(eventloop* loop, poller::POLLER p) {
	if (p == kEPOLL)
		return new kepoll(loop);
	else
		return nullptr;
}
