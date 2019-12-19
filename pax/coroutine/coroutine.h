#pragma once

#include<pax/base/thread.h>

#include<ucontext.h>

namespace pax {

class coroutine_item;

class coroutine :uncopyable {
	friend class coroutine_item;
	friend class threadlocal<coroutine>;
public:

	inline static void yield()noexcept(false);//std::logic_error

protected:

	coroutine();
	~coroutine();

private:

	static coroutine* threadCoenv();
	static threadlocal<coroutine> thread_coenv_;

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

	coroutine_item(const functor& func, bool shared = false)noexcept(false);//std::bad_alloc
	coroutine_item(functor&& func, bool shared = false)noexcept(false);//std::bad_alloc
	~coroutine_item();//可以考虑换成virtual

	costate getState()const { return state_; }
	inline void resume()noexcept(false);//std::logic_error, std::bad_alloc

private:
	enum stacksize {
		kStackSize = 64 * 1024,
		kSharedStackSize = 1024 * 1024
	};

	static void coroutineFunc(coroutine_item* co);

	static void makeContext(coroutine_item* co);
	static void swapContext(coroutine_item* curr, coroutine_item* pend);

	static threadlocal<char> shared_stack_;
	thread_local static coroutine_item* running_crt_;

	costate state_;
	functor coFunc;

	ucontext_t ctx_;

	bool shared_;
	char* stack_bp_;
	char* stack_sp_;
	char* stack_buff_;
	size_t length_;

};

void coroutine::yield() {
	threadCoenv()->yieldFunc();
}

void coroutine_item::resume() {
	coroutine::threadCoenv()->resumeFunc(this);
}

}//namespace pax