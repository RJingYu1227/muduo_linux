#pragma once

#include"kthread.h"

#include<ucontext.h>
#include<map>

typedef unsigned int coroutine_t;

class coroutine :uncopyable {
public:
	typedef std::function<void()> functor;

	static coroutine* threadCoenv();
	static void freeCoenv();

	coroutine_t create(const functor& func);
	coroutine_t create(functor&& func);
	void free(coroutine_t id);
	//void cancel(coroutine_t id);

	void resume(coroutine_t id);
	void yield();

	coroutine_t self();

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

	void makeCtx(impl* co);
	static void coroutineFunc(impl* co);

	static kthreadlocal<coroutine> thread_coenv_;

	std::map<coroutine_t, impl*> comap_;
	ucontext_t env_ctx_;

	impl* costack_[128];
	int sindex_;
	coroutine_t coid_;

};

