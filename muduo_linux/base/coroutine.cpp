﻿#include "coroutine.h"

#include<malloc.h>
#include<assert.h>

kthreadlocal<coroutine> coroutine::thread_coenv_(coroutine::freeCoenv);

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

void coroutine::coroutineFunc(impl* co) {
	assert(co->state_ == FREE);
	co->state_ = RUNNING;
	co->coFunc();
	co->state_ = DONE;

	threadCoenv()->yieldFunc();
}

coroutine::coroutine()
	:sindex_(-1),
	coid_(0) {

	getcontext(&env_ctx_);
}

coroutine::~coroutine() {
	assert(sindex_ == -1);
	impl* co;
	for (auto iter = comap_.begin(); iter != comap_.end(); ++iter) {
		co = iter->second;
		::free(co->ctx_.uc_stack.ss_sp);
		delete co;//注意这里
	}
}

void coroutine::makeCtx(impl* co) {
	getcontext(&co->ctx_);

	co->ctx_.uc_flags = env_ctx_.uc_flags;
	co->ctx_.uc_link = 0;
	co->ctx_.uc_sigmask = env_ctx_.uc_sigmask;

	void* sp = malloc(64 * 1024);
	//memset(sp, 0, 64 * 1024);

	co->ctx_.uc_stack.ss_sp = sp;
	co->ctx_.uc_stack.ss_size = 64 * 1024;
	co->ctx_.uc_stack.ss_flags = 0;

	makecontext(&co->ctx_, (void(*)())coroutine::coroutineFunc, 1, co);
}

coroutine_t coroutine::createFunc(const functor& func) {
	impl* co = new impl();
	++coid_;
	co->id_ = coid_;
	co->coFunc = func;
	makeCtx(co);

	comap_[coid_] = co;
	return coid_;
}

coroutine_t coroutine::createFunc(functor&& func) {
	impl* co = new impl();
	++coid_;
	co->id_ = coid_;
	co->coFunc = std::move(func);
	makeCtx(co);

	comap_[coid_] = co;
	return coid_;
}

void coroutine::freeFunc(coroutine_t id) {
	auto iter = comap_.find(id);
	assert(iter != comap_.end());
	
	impl* co = iter->second;
	assert(co->state_ == FREE || co->state_ == DONE);

	::free(co->ctx_.uc_stack.ss_sp);
	delete co;
	comap_.erase(id);
}

void coroutine::resumeFunc(coroutine_t id) {
	auto iter = comap_.find(id);
	assert(iter != comap_.end());
	
	impl* pendco = iter->second;
	assert(pendco->state_ == FREE || pendco->state_ == SUSPEND);
	if (pendco->state_ == SUSPEND)
		pendco->state_ = RUNNING;

	if (sindex_ == -1) {
		++sindex_;
		costack_[sindex_] = pendco;
		swapcontext(&env_ctx_, &pendco->ctx_);
	}
	else {
		//注意这里
		assert(sindex_ < 127);
		impl* currco = costack_[sindex_];
		currco->state_ = SUSPEND;
		++sindex_;
		costack_[sindex_] = pendco;
		swapcontext(&currco->ctx_, &pendco->ctx_);
	}
}

void coroutine::yieldFunc() {
	assert(sindex_ != -1);

	impl* thisco = costack_[sindex_];
	--sindex_;
	if (thisco->state_ == RUNNING)
		thisco->state_ = SUSPEND;

	if (sindex_ == -1)
		swapcontext(&thisco->ctx_, &env_ctx_);
	else {
		impl* lastco = costack_[sindex_];
		lastco->state_ = RUNNING;
		swapcontext(&thisco->ctx_, &lastco->ctx_);
	}
}

coroutine_t coroutine::selfFunc() {
	if (sindex_ == -1)
		return 0;
	else
		return costack_[sindex_]->id_;
}