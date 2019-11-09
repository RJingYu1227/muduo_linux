#include "coroutine.h"

#include<malloc.h>
#include<assert.h>

kthreadlocal<coroutine> coroutine::thread_coenv_(coroutine::freeCoenv);

coroutine::coroutine()
	:sindex_(-1) {

	::getcontext(&env_ctx_);
}

coroutine::~coroutine() {
	assert(sindex_ == -1);
}

coroutine* coroutine::threadCoenv() {
	coroutine* env = thread_coenv_.get();
	if (env == nullptr) {
		env = new coroutine();
		thread_coenv_.set(env);
	}

	return env;
}

void coroutine::freeCoenv(void* ptr) {
	coroutine* env = (coroutine*)ptr;
	delete env;
}

void coroutine::coroutine_item::coroutineFunc(coroutine_item* co) {
	assert(co->state_ == FREE);
	co->state_ = RUNNING;
	co->coFunc();
	co->state_ = DONE;

	coroutine::yield();
}

void coroutine::coroutine_item::makeContext(coroutine::coroutine_item* co) {
	::getcontext(&co->ctx_);

	co->ctx_.uc_link = 0;

	void* sp = ::malloc(64 * 1024);
	//memset(sp, 0, 64 * 1024);
	co->ctx_.uc_stack.ss_sp = sp;
	co->ctx_.uc_stack.ss_size = 64 * 1024;
	co->ctx_.uc_stack.ss_flags = 0;

	::makecontext(&co->ctx_, (void(*)())coroutineFunc, 1, co);
}

coroutine::coroutine_item::coroutine_item(const functor& func) :
	state_(FREE),
	coFunc(func) {

	makeContext(this);
}

coroutine::coroutine_item::coroutine_item(functor&& func) :
	state_(FREE),
	coFunc(std::move(func)) {

	makeContext(this);
}

coroutine::coroutine_item::~coroutine_item() {
	assert(state_ == FREE || state_ == DONE);
	::free(ctx_.uc_stack.ss_sp);
}

void coroutine::resumeFunc(coroutine_item* co) {
	assert(co->state_ == coroutine_item::FREE || co->state_ == coroutine_item::SUSPEND);

	if (co->state_ == coroutine_item::SUSPEND)
		co->state_ = coroutine_item::RUNNING;

	if (sindex_ == -1) {
		++sindex_;
		costack_[sindex_] = co;
		::swapcontext(&env_ctx_, &co->ctx_);
	}
	else {
		//注意这里
		assert(sindex_ < 127);
		coroutine_item* currco = costack_[sindex_];
		currco->state_ = coroutine_item::SUSPEND;
		++sindex_;
		costack_[sindex_] = co;
		::swapcontext(&currco->ctx_, &co->ctx_);
	}
}

void coroutine::yieldFunc() {
	assert(sindex_ != -1);

	coroutine_item* thisco = costack_[sindex_];
	--sindex_;
	if (thisco->state_ == coroutine_item::RUNNING)
		thisco->state_ = coroutine_item::SUSPEND;

	if (sindex_ == -1)
		::swapcontext(&thisco->ctx_, &env_ctx_);
	else {
		coroutine_item* lastco = costack_[sindex_];
		lastco->state_ = coroutine_item::RUNNING;
		::swapcontext(&thisco->ctx_, &lastco->ctx_);
	}
}