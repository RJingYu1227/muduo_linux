#include"poller.h"
#include"kepoll.h"
#include"kpoll.h"

poller* poller::newPoller(eventloop* loop, poller::POLLER p) {
	if (p == kEPOLL)
		return new kepoll(loop);
	else if (p == kPOLL)
		return new kpoll(loop);
	else
		return nullptr;
}
