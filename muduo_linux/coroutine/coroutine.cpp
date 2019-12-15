#include"coroutine.h"

#include<malloc.h>
#include<assert.h>
#include<string.h>

kthreadlocal<coroutine> coroutine::thread_coenv_(kthreadlocal<coroutine>::freeFunc);
kthreadlocal<char> coroutine_item::shared_stack_(kthreadlocal<char>::freeFunc);
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

	coroutine::yield();//这里不会throw
}

void coroutine_item::makeContext(coroutine_item* co) {
	::getcontext(&co->ctx_);

	co->ctx_.uc_link = 0;
	co->ctx_.uc_stack.ss_flags = 0;

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

	if (curr->shared_ && curr->state_ == SUSPEND) {
		curr->stack_sp_ = &addr;
		size_t len = curr->stack_bp_ - curr->stack_sp_;
		if (curr->length_ < len) {
			if (curr->stack_buff_) {
				::free(curr->stack_buff_);

				curr->stack_buff_ = nullptr;
				curr->length_ = 0;
			}

			curr->stack_buff_ = (char*)::malloc(len);
			if (curr->stack_buff_ == nullptr)
				throw std::bad_alloc();
		}

		curr->length_ = len;
		memcpy(curr->stack_buff_, curr->stack_sp_, curr->length_);
	}

	if (pend->shared_) {
		if (pend->ctx_.uc_stack.ss_sp != shared_stack_.get())
			throw std::logic_error("共享栈协程不能跨线程调用");
			
		if (pend->state_ == FREE)
			::makecontext(&pend->ctx_, (void(*)())coroutineFunc, 1, pend);
	}

	running_crt_ = pend;

	::swapcontext(&curr->ctx_, &pend->ctx_);

	coroutine_item*  co = running_crt_;
	if (co->shared_)
		memcpy(co->stack_sp_, co->stack_buff_, co->length_);
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

	if (stack_buff_)
		::free(stack_buff_);
}

void coroutine::resumeFunc(coroutine_item* co) {
	if (co->state_ == coroutine_item::RUNNING)
		throw std::logic_error("协程正在执行当中");

	if (co->state_ == coroutine_item::DONE)
		throw std::logic_error("协程已经执行完毕");

	if (index_ == 128)
		throw std::logic_error("协程调用栈溢出，index_ == 128");
	
	coroutine_item* curr = call_stack_[index_];
	curr->state_ = coroutine_item::SUSPEND;

	++index_;

	call_stack_[index_] = co;
	if (co->state_ == coroutine_item::SUSPEND)
		co->state_ = coroutine_item::RUNNING;

	try {
		coroutine_item::swapContext(curr, co);
	}
	catch (...) {
		if (co->state_ == coroutine_item::RUNNING)
			co->state_ = coroutine_item::SUSPEND;

		--index_;

		curr->state_ = coroutine_item::RUNNING;

		throw;
	}
}

void coroutine::yieldFunc() {
	if (index_ == 0)
		throw std::logic_error("协程调用栈溢出，index_ == 0");

	coroutine_item* curr = call_stack_[index_];
	if (curr->state_ == coroutine_item::RUNNING)
		curr->state_ = coroutine_item::SUSPEND;

	--index_;

	coroutine_item* pend = call_stack_[index_];
	pend->state_ = coroutine_item::RUNNING;

	try {
		//如果curr->state_ == coroutine_item::DONE，则不会出错
		coroutine_item::swapContext(curr, pend);
	}
	catch (...) {
		pend->state_ = coroutine_item::SUSPEND;

		++index_;

		curr->state_ = coroutine_item::RUNNING;

		throw;
	}
}