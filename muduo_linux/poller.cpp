#include"poller.h"
#include"epoller.h"
#include"eventloop.h"

poller::poller(eventloop* loop)
	:loop_(loop) {

}

poller::~poller() = default;

inline void poller::assertInLoopThread() {
	loop_->assertInLoopThread();
}

poller* poller::newPoller(eventloop* loop, poller::POLLER p) {
	if (p == kEPOLL)
		return new epoller(loop);
	else
		return nullptr;
}
