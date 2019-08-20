#include "coroutine.h"

#include<malloc.h>
#include<assert.h>

thread_local coroutine* coroutine::co_env_ = nullptr;

void coroutine::coroutineFunc(impl* co) {
	if (co->state_ == FREE) {
		co->state_ = RUNNING;
		co->coFunc();
	}
	co->state_ = DONE;

	coroutine* env = instance();
	env->yield();
}

coroutine::coroutine()
	:coid_(0) {

	getcontext(&env_ctx_);
}

coroutine::~coroutine() {
	impl* co;
	for (auto iter = comap_.begin(); iter != comap_.end(); ++iter) {
		co = iter->second;
		::free(co->ctx_.uc_stack.ss_sp);
		::free(co);
	}
}

void coroutine::makeCtx(impl* co) {
	co->ctx_.uc_flags = env_ctx_.uc_flags;
	co->ctx_.uc_link = 0;
	co->ctx_.uc_sigmask = env_ctx_.uc_sigmask;

	co->ctx_.uc_stack.ss_sp = malloc(64 * 1024);
	co->ctx_.uc_stack.ss_size = 64 * 1024;
	co->ctx_.uc_stack.ss_flags = 0;

	//注意这里
	co->ctx_.uc_mcontext.fpregs = &co->ctx_.__fpregs_mem;
	makecontext(&co->ctx_, (void(*)())coroutine::coroutineFunc, 1, co);
}

coroutine_t coroutine::create(const functor& func) {
	impl* co = (impl*)malloc(sizeof(impl));
	++coid_;
	co->id_ = coid_;
	co->coFunc = func;
	makeCtx(co);

	comap_[coid_] = co;
	return coid_;
}

coroutine_t coroutine::create(functor&& func) {
	impl* co = (impl*)malloc(sizeof(impl));
	++coid_;
	co->id_ = coid_;
	co->coFunc = std::move(func);
	makeCtx(co);

	comap_[coid_] = co;
	return coid_;
}

void coroutine::free(coroutine_t id) {
	auto iter = comap_.find(id);
	assert(iter != comap_.end());
	
	impl* co = iter->second;
	assert(co->state_ == FREE || co->state_ == DONE);

	::free(co->ctx_.uc_stack.ss_sp);
	::free(co);
	comap_.erase(id);
}

void coroutine::resume(coroutine_t id) {
	auto iter = comap_.find(id);
	assert(iter != comap_.end());
	
	impl* pendco = iter->second;
	assert(pendco->state_ != DONE);
	if (pendco->state_ == SUSPEND)
		pendco->state_ = RUNNING;

	if (costack_.empty()) {
		costack_.push(pendco);
		swapcontext(&env_ctx_, &pendco->ctx_);
	}
	else {
		impl* currco = costack_.top();
		currco->state_ = SUSPEND;
		costack_.push(pendco);
		swapcontext(&currco->ctx_, &pendco->ctx_);
	}
}

void coroutine::yield() {
	assert(!costack_.empty());

	impl* thisco = costack_.top();
	costack_.pop();
	if (thisco->state_ == RUNNING)
		thisco->state_ = SUSPEND;

	if (costack_.empty())
		swapcontext(&thisco->ctx_, &env_ctx_);
	else {
		impl* lastco = costack_.top();
		lastco->state_ = RUNNING;
		swapcontext(&thisco->ctx_, &lastco->ctx_);
	}
}
