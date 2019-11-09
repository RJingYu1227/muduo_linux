#pragma once

#include"blockqueue.h"
#include"coroutine.h"
#include"coevent.h"

#include<atomic>

class coservice :uncopyable {
private:

	struct impl {
		impl() {}

		impl(void* ptr) :
			inqueue_(0),
			done_(0),
			ptr_(ptr) {

		}

		bool inqueue_;
		bool done_;
		void* ptr_;
	};

public:
	typedef std::function<void()> functor;

	coservice();
	~coservice();

	size_t run();

	class coservice_item :
		coroutine::coroutine_item,
		public coevent {
		friend class coservice;
	public:

		coservice_item(int fd, const functor& func, coservice* service);
		coservice_item(int fd, functor&& func, coservice* service);
		~coservice_item();

		inline void updateEvents();
		//暂时不适合使用边沿触发模式，请参考coservice::run
		//使用ET模式时，如果其它线程执行epoll_wait得到对应fd的返回事件
		//但是，此时对应协程已经被添加进queue_（由其它类型的事件触发）或者正在执行且后续并没有处理该事件
		//会使该事件很难得到处理，可以参考boost::asio的 事件任务队列 进行更改

	private:

	

		static void coroutineFunc(coservice_item* cst);

		coservice* service_;
		impl* tie_;
		functor Func;

	};

private:

	void add(coservice_item* cst);
	void modify(coservice_item* cst);
	void remove(coservice_item* cst);

	std::atomic_int32_t item_count_;

	int epfd_;
	std::vector<epoll_event> revents_;

	blockqueue<impl*> queue_;

};

void coservice::coservice_item::updateEvents() {
	service_->modify(this);
}
