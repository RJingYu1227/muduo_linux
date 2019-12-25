#pragma once

#include<pax/base/uncopyable.h>

#include<ucontext.h>

#include<functional>

namespace pax {

class coroutine_item;

class coroutine :uncopyable {
	friend class coroutine_item;
public:

	static void yield()noexcept(false);

	~coroutine();

protected:

	coroutine();

private:

	static coroutine* threadCoenv();

	void resumeFunc(coroutine_item* co);
	void yieldFunc();

	coroutine_item* env_co_;

	coroutine_item* call_stack_[129];
	size_t index_;

};

//目前实现的共享栈协程，不能跨线程调用
class coroutine_item :uncopyable {
	friend class coroutine;
public:
	typedef std::function<void()> functor;

	enum costate {
		FREE,
		RUNNING,
		INSTACK,
		SUSPEND,
		DONE,
	};

	explicit coroutine_item(const functor& func, bool shared = false)noexcept(false);//std::bad_alloc
	explicit coroutine_item(functor&& func, bool shared = false)noexcept(false);//std::bad_alloc
	~coroutine_item();//可以考虑换成virtual

	costate getState()const { return state_; }
	void resume()noexcept(false);

private:

	static void coroutineFunc(coroutine_item* co);

	static void makeContext(coroutine_item* co);
	static void swapContext(coroutine_item* curr, coroutine_item* pend);

	thread_local static coroutine_item* running_crt_;

	coroutine_item();//for coroutine

	costate state_;
	functor coFunc;

	ucontext_t ctx_;

	const bool shared_;
	char* stack_sp_;
	char* stack_buff_;
	size_t length_;

};

//std::logic_error, std::bad_alloc
inline void coroutine::yield()noexcept(false) {
	threadCoenv()->yieldFunc();
}

//std::logic_error, std::bad_alloc
inline void coroutine_item::resume()noexcept(false) {
	coroutine::threadCoenv()->resumeFunc(this);
}

}//namespace pax