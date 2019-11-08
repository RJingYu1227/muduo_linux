#pragma once

#include"kthread.h"

#include<ucontext.h>

class coroutine :uncopyable {
public:
	typedef std::function<void()> functor;

	inline static void yield();

	class coroutine_item :uncopyable {
		friend class coroutine;
	public:

		enum costate {
			FREE,
			RUNNING,
			SUSPEND,
			DONE,
		};

		coroutine_item(const functor& func);
		coroutine_item(functor&& func);
		~coroutine_item();//暂时不需要virtual

		costate getState()const { return state_; }

		inline void resume();
		inline void yield();

	private:

		static void coroutineFunc(coroutine_item* co);
		static void makeContext(coroutine_item* co);

		costate state_;
		functor coFunc;
		ucontext_t ctx_;

	};

protected:

	coroutine();
	~coroutine();

private:

	static coroutine* threadCoenv();
	static void freeCoenv(void* ptr);

	static kthreadlocal<coroutine> thread_coenv_;

	void resumeFunc(coroutine_item* co);
	void yieldFunc();

	ucontext_t env_ctx_;

	coroutine_item* costack_[128];
	int sindex_;

};

void coroutine::yield() {
	threadCoenv()->yieldFunc();
}

void coroutine::coroutine_item::resume() {
	threadCoenv()->resumeFunc(this);
}

void coroutine::coroutine_item::yield() {
	threadCoenv()->yieldFunc();
}
