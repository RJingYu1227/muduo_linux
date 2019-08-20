#pragma once

#include"uncopyable.h"

#include<ucontext.h>
#include<map>
#include<stack>
#include<functional>

typedef unsigned int coroutine_t;

class coroutine :uncopyable {
public:
	typedef std::function<void()> functor;

	enum costate {
		FREE,
		RUNNING,
		SUSPEND,
		DONE,
	};

	static coroutine* instance() {
		static coroutine ins;
		co_env_ = &ins;
		return co_env_;
	}

	coroutine_t create(const functor& func);
	coroutine_t create(functor&& func);
	void free(coroutine_t id);
	//void cancel(coroutine_t id);

	void resume(coroutine_t id);
	void yield();

protected:

	coroutine();
	~coroutine();

private:

	thread_local static coroutine* co_env_;

	struct impl {
		coroutine_t id_;
		costate state_ = FREE;
		functor coFunc;

		ucontext_t ctx_;
	};

	void makeCtx(impl* co);
	static void coroutineFunc(impl* co);

	std::map<coroutine_t, impl*> comap_;
	std::stack<impl*> costack_;
	ucontext_t env_ctx_;
	coroutine_t coid_;

};

