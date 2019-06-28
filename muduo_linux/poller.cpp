#include"poller.h"
#include"epoller.h"

poller* poller::newPoller(eventloop* loop, poller::POLLER p) {
	if (p == kEPOLL)
		return new epoller(loop);
	else
		return nullptr;
}
