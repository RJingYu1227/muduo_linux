﻿#include<pax/coroutine/coroutine.h>

#include<malloc.h>
#include<assert.h>
#include<string.h>

using namespace pax;

namespace {

enum config {
	kStackSize = 64 * 1024,
	kSharedStackSize = 1024 * 1024
};

//共享栈协程可能要先于环境协程创建出来
thread_local char* shared_stack = nullptr;
thread_local coroutine* env = nullptr;

class free {
public:

	~free() {
		if (shared_stack)
			::free(shared_stack);

		if (env)
			delete env;
	}

};

thread_local free obj;

}

thread_local coroutine_item* coroutine_item::running_crt_ = nullptr;

coroutine::coroutine() :
	env_co_((coroutine_item*)::malloc(sizeof(coroutine_item))),
	index_(0) {

	assert(env_co_);

	env_co_->shared_ = false;
	call_stack_[index_] = env_co_;
	env_co_->state_ = coroutine_item::RUNNING;
	coroutine_item::running_crt_ = env_co_;
	//::getcontext(&env_co_->ctx_);
}

coroutine::~coroutine() {
	assert(index_ == 0);

	::free(env_co_);
}

coroutine* coroutine::threadCoenv() {
	if (env == nullptr)
		env = new coroutine();

	return env;
}

void coroutine_item::coroutineFunc(coroutine_item* co) {
	assert(co->state_ == FREE);
	co->state_ = RUNNING;

	co->coFunc();

	co->state_ = DONE;

	coroutine::yield();//这里不会throw
}

void coroutine_item::makeContext(coroutine_item* co) {
	::getcontext(&co->ctx_);

	co->ctx_.uc_link = 0;
	co->ctx_.uc_stack.ss_flags = 0;

	void* sp;
	if (co->shared_) {
		if (shared_stack == nullptr) {
			shared_stack = (char*)::malloc(kSharedStackSize);
			if (shared_stack == nullptr)
				throw std::bad_alloc();
		}
		sp = shared_stack;

		co->stack_bp_ = (char*)sp + kSharedStackSize;
		co->ctx_.uc_stack.ss_size = kSharedStackSize;
		co->ctx_.uc_stack.ss_sp = sp;
	}
	else {
		sp = ::malloc(kStackSize);
		if (sp == nullptr)
			throw std::bad_alloc();

		co->stack_bp_ = (char*)sp + kStackSize;
		co->ctx_.uc_stack.ss_size = kStackSize;
		co->ctx_.uc_stack.ss_sp = sp;

		::makecontext(&co->ctx_, (void(*)())coroutineFunc, 1, co);
	}
	
}

void coroutine_item::swapContext(coroutine_item* curr, coroutine_item* pend) {
	char addr;

	if (curr->shared_ && curr->state_ != DONE) {
		curr->stack_sp_ = &addr;
		size_t len = curr->stack_bp_ - curr->stack_sp_;

		curr->stack_buff_ = (char*)::malloc(len);
		curr->length_ = len;
		if (curr->stack_buff_ == nullptr)
			throw std::bad_alloc();

		memcpy(curr->stack_buff_, curr->stack_sp_, curr->length_);
	}

	if (pend->shared_) {
		if (pend->ctx_.uc_stack.ss_sp != shared_stack)
			throw std::logic_error("共享栈协程不能跨线程调用");
			
		if (pend->state_ == FREE)
			::makecontext(&pend->ctx_, (void(*)())coroutineFunc, 1, pend);
	}

	running_crt_ = pend;

	::swapcontext(&curr->ctx_, &pend->ctx_);

	coroutine_item*  co = running_crt_;

	if (co->shared_) {
		memcpy(co->stack_sp_, co->stack_buff_, co->length_);

		::free(co->stack_buff_);
		co->stack_buff_ = nullptr;
		co->length_ = 0;
	}
}

coroutine_item::coroutine_item(const functor& func, bool shared) :
	state_(FREE),
	coFunc(func),
	shared_(shared),
	stack_bp_(0),
	stack_sp_(0),
	stack_buff_(0),
	length_(0) {

	makeContext(this);
}

coroutine_item::coroutine_item(functor&& func, bool shared) :
	state_(FREE),
	coFunc(std::move(func)),
	shared_(shared),
	stack_bp_(0),
	stack_sp_(0),
	stack_buff_(0),
	length_(0) {

	makeContext(this);
}

coroutine_item::~coroutine_item() {
	assert(state_ == FREE || state_ == DONE);

	if (shared_ == false)
		::free(ctx_.uc_stack.ss_sp);
}

void coroutine::resumeFunc(coroutine_item* co) {
	if (index_ == 128)
		throw std::logic_error("协程调用栈溢出，index_ == 128");

	coroutine_item::costate last_state = co->state_;
	switch (last_state) {
	case(coroutine_item::FREE):

		break;
	case(coroutine_item::RUNNING):
		throw std::logic_error("协程正在执行当中");

		break;
	case(coroutine_item::INSTACK):
		throw std::logic_error("协程正在调用栈中");

		break;
	case(coroutine_item::SUSPEND):
		co->state_ = coroutine_item::RUNNING;

		break;
	case(coroutine_item::DONE):
		throw std::logic_error("协程已经执行完毕");

		break;
	}
	
	coroutine_item* curr = call_stack_[index_];
	curr->state_ = coroutine_item::INSTACK;

	++index_;

	call_stack_[index_] = co;

	try {
		coroutine_item::swapContext(curr, co);
	}
	catch (...) {
		co->state_ = last_state;

		--index_;

		curr->state_ = coroutine_item::RUNNING;

		throw;
	}
}

void coroutine::yieldFunc() {
	if (index_ == 0)
		throw std::logic_error("协程调用栈溢出，index_ == 0");

	coroutine_item* curr = call_stack_[index_];

	--index_;

	coroutine_item* pend = call_stack_[index_];
	pend->state_ = coroutine_item::RUNNING;

	switch (curr->state_) {
	case(coroutine_item::RUNNING):
		curr->state_ = coroutine_item::SUSPEND;

		try {
			coroutine_item::swapContext(curr, pend);
		}
		catch (...) {
			pend->state_ = coroutine_item::INSTACK;

			++index_;

			curr->state_ = coroutine_item::RUNNING;

			throw;
		}

		break;
	case(coroutine_item::DONE):
		coroutine_item::swapContext(curr, pend);

		break;
	default:

		break;
	}
}