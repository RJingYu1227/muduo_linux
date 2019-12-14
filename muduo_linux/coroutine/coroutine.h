#pragma once

#include"kthread.h"

#include<ucontext.h>

class coroutine_item;

class coroutine :uncopyable {
	friend class coroutine_item;
	friend class kthreadlocal<coroutine>;
public:

	inline static void yield();

protected:

	coroutine();
	~coroutine();

private:

	static coroutine* threadCoenv();
	static kthreadlocal<coroutine> thread_coenv_;

	void resumeFunc(coroutine_item* co);
	void yieldFunc();

	coroutine_item* env_co_;

	coroutine_item* call_stack_[128];
	int index_;

};

//目前实现的共享栈使用条件比较苛刻，不能跨线程，不能在一个共享栈的协程执行过程当中创建一个新的共享栈协程
class coroutine_item :uncopyable {
	friend class coroutine;
public:
	typedef std::function<void()> functor;

	enum costate {
		FREE,
		RUNNING,
		SUSPEND,
		DONE,
	};

	coroutine_item(const functor& func, bool shared = false);
	coroutine_item(functor&& func, bool shared = false);
	~coroutine_item();//可以考虑换成virtual

	costate getState()const { return state_; }
	inline void resume();

private:
	enum stacksize {
		kStackSize = 64 * 1024,
		kSharedStackSize = 1024 * 1024
	};

	static void coroutineFunc(coroutine_item* co);

	//std::bad_alloc, std::logic_error
	static void makeContext(coroutine_item* co)noexcept(false);
	static void swapContext(coroutine_item* curr, coroutine_item* pend)noexcept(false);

	static kthreadlocal<char> shared_stack_;
	thread_local static coroutine_item* running_crt_;

	costate state_;
	functor coFunc;

	ucontext_t ctx_;

	bool shared_;
	char* stack_bp_;
	char* stack_sp_;
	char* stack_buff_;
	size_t buff_len_;

};

void coroutine::yield() {
	threadCoenv()->yieldFunc();
}

void coroutine_item::resume() {
	coroutine::threadCoenv()->resumeFunc(this);
}