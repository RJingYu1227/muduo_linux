#pragma once

#include"kthread.h"

#include<ucontext.h>
#include<map>

typedef unsigned int coroutine_t;

class coroutine :uncopyable {
public:
	typedef std::function<void()> functor;

	inline static coroutine_t create(const functor& func);
	inline static coroutine_t create(functor&& func);
	inline static void free(coroutine_t id);
	//inline static void cancel(coroutine_t id);

	inline static void resume(coroutine_t id);
	inline static void yield();

	inline static coroutine_t self();

protected:

	coroutine();
	~coroutine();

private:
	enum costate {
		FREE,
		RUNNING,
		SUSPEND,
		DONE,
	};

	struct impl {
		coroutine_t id_;
		costate state_ = FREE;
		functor coFunc;

		ucontext_t ctx_;
	};

	static coroutine* threadCoenv();
	static void freeCoenv(void* ptr);
	static void coroutineFunc(impl* co);

	static kthreadlocal<coroutine> thread_coenv_;

	coroutine_t createFunc(const functor& func);
	coroutine_t createFunc(functor&& func);
	void freeFunc(coroutine_t id);
	void resumeFunc(coroutine_t id);
	void yieldFunc();
	coroutine_t selfFunc();

	void makeCtx(impl* co);

	std::map<coroutine_t, impl*> comap_;
	ucontext_t env_ctx_;

	impl* costack_[128];
	int sindex_;
	coroutine_t coid_;

};

coroutine_t coroutine::create(const functor& func) {
	return threadCoenv()->createFunc(func);
}

coroutine_t coroutine::create(functor&& func) {
	return threadCoenv()->createFunc(std::move(func));
}

void coroutine::free(coroutine_t id) {
	threadCoenv()->freeFunc(id);
}

void coroutine::resume(coroutine_t id) {
	threadCoenv()->resumeFunc(id);
}

void coroutine::yield() {
	threadCoenv()->yieldFunc();
}

coroutine_t coroutine::self() {
	return threadCoenv()->selfFunc();
}