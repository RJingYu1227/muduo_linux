#include "coroutine.h"

#include<malloc.h>
#include<assert.h>
#include<string.h>

kthreadlocal<coroutine> coroutine::thread_coenv_(kthreadlocal<coroutine>::freeFunc);
kthreadlocal<char> coroutine_item::shared_stack_(kthreadlocal<char>::freeFunc);
thread_local coroutine_item* coroutine_item::running_crt_ = nullptr;

coroutine::coroutine() :
	env_co_((coroutine_item*)::malloc(sizeof(coroutine_item))),
	index_(-1) {

	assert(env_co_);
	env_co_->shared_ = false;
	//::getcontext(&env_co_->ctx_);
}

coroutine::~coroutine() {
	assert(index_ == -1);

	::free(env_co_);
}

coroutine* coroutine::threadCoenv() {
	coroutine* env = thread_coenv_.get();
	if (env == nullptr) {
		env = new coroutine();
		thread_coenv_.set(env);
	}

	return env;
}

void coroutine_item::coroutineFunc(coroutine_item* co) {
	assert(co->state_ == FREE);
	co->state_ = RUNNING;
	co->coFunc();
	co->state_ = DONE;

	coroutine::yield();
}

void coroutine_item::makeContext(coroutine_item* co) {
	::getcontext(&co->ctx_);

	co->ctx_.uc_link = 0;

	void* sp;
	if (co->shared_) {
		sp = shared_stack_.get();
		if (sp == nullptr) {
			sp = ::malloc(kSharedStackSize);
			if (sp == nullptr)
				throw std::bad_alloc();

			shared_stack_.set((const char*)sp);
		}
		co->stack_bp_ = (char*)sp + kSharedStackSize;
		co->ctx_.uc_stack.ss_size = kSharedStackSize;
	}
	else {
		sp = ::malloc(kStackSize);
		if (sp == nullptr)
			throw std::bad_alloc();

		co->stack_bp_ = (char*)sp + kStackSize;
		co->ctx_.uc_stack.ss_size = kStackSize;
	}
	co->ctx_.uc_stack.ss_sp = sp;
	co->ctx_.uc_stack.ss_flags = 0;

	::makecontext(&co->ctx_, (void(*)())coroutineFunc, 1, co);
}

void coroutine_item::swapContext(coroutine_item* curr, coroutine_item* pend) {
	char addr;
	curr->stack_sp_ = &addr;

	if (curr->shared_) {
		size_t len = curr->stack_bp_ - curr->stack_sp_;
		if (curr->stack_buff_ == nullptr || curr->buff_len_ < len) {
			if (curr->stack_buff_)
				::free(curr->stack_buff_);

			curr->stack_buff_ = (char*)::malloc(len);
			if (curr->stack_buff_ == nullptr)
				throw std::bad_alloc();
		}

		curr->buff_len_ = len;
		memcpy(curr->stack_buff_, curr->stack_sp_, curr->buff_len_);
	}

	running_crt_ = pend;
	::swapcontext(&curr->ctx_, &pend->ctx_);

	coroutine_item*  co = running_crt_;
	if (co->shared_)
		memcpy(co->stack_sp_, co->stack_buff_, co->buff_len_);
}

coroutine_item::coroutine_item(const functor& func, bool shared) :
	state_(FREE),
	coFunc(func),
	shared_(shared),
	stack_bp_(0),
	stack_sp_(0),
	stack_buff_(0),
	buff_len_(0) {

	makeContext(this);
}

coroutine_item::coroutine_item(functor&& func, bool shared) :
	state_(FREE),
	coFunc(std::move(func)),
	shared_(shared),
	stack_bp_(0),
	stack_sp_(0),
	stack_buff_(0),
	buff_len_(0) {

	makeContext(this);
}

coroutine_item::~coroutine_item() {
	assert(state_ == FREE || state_ == DONE);

	if (shared_ == false)
		::free(ctx_.uc_stack.ss_sp);

	if (stack_buff_)
		::free(stack_buff_);
}

void coroutine::resumeFunc(coroutine_item* co) {
	assert(co->state_ == coroutine_item::FREE || co->state_ == coroutine_item::SUSPEND);

	if (co->state_ == coroutine_item::SUSPEND)
		co->state_ = coroutine_item::RUNNING;

	if (index_ == -1) {
		++index_;
		call_stack_[index_] = co;
		coroutine_item::swapContext(env_co_, co);
	}
	else {
		//注意这里
		assert(index_ < 127);
		coroutine_item* currco = call_stack_[index_];
		currco->state_ = coroutine_item::SUSPEND;
		++index_;
		call_stack_[index_] = co;
		coroutine_item::swapContext(currco, co);
	}
}

void coroutine::yieldFunc() {
	assert(index_ != -1);

	coroutine_item* thisco = call_stack_[index_];
	--index_;
	if (thisco->state_ == coroutine_item::RUNNING)
		thisco->state_ = coroutine_item::SUSPEND;

	if (index_ == -1)
		coroutine_item::swapContext(thisco, env_co_);
	else {
		coroutine_item* lastco = call_stack_[index_];
		lastco->state_ = coroutine_item::RUNNING;
		coroutine_item::swapContext(thisco, lastco);
	}
}