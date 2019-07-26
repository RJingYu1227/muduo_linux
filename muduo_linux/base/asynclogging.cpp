#include"asynclogging.h"
#include"logfile.h"
#include"blockqueue.h"

#include<assert.h>

namespace {

	typedef blockqueue<l_logbuffer*> buffer_queue;

	struct asyncimpl {
		asyncimpl(l_logbuffer* l_logbuffer, buffer_queue* queue, logfile* output) :
			buffer_(l_logbuffer),
			queue_(queue),
			output_(output)
		{}

		l_logbuffer* buffer_ = nullptr;
		buffer_queue* queue_ = nullptr;
		logfile* output_ = nullptr;
	};

	blockqueue<asyncimpl> async_impls_;

}

asynclogger::asynclogger(const char* basename, off_t rollsize)
	:basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogger::threadFunc, this)),
	running_(0) {

}

void asynclogger::append(const s_logbuffer& sbuff) {
	thread_local logfile output_
	((basename_ + std::to_string(pthread_self()) + '.').c_str(), rollsize_);
	thread_local buffer_queue thread_buffers_;
	thread_local asyncimpl impl_(nullptr, &thread_buffers_, &output_);
	thread_local time_t last_put_ = ::time(NULL);

	l_logbuffer* pbuf = nullptr;
	size_t size = thread_buffers_.size();
	size_t length = sbuff.length();

	if (size == 0)
		pbuf = new l_logbuffer;
	else {
		pbuf = thread_buffers_.take_front();
		impl_.buffer_ = pbuf;

		if (pbuf->leftBytes() >= length) {
			pbuf->append(sbuff.getData(), length);
			if (sbuff.getTime() - last_put_ >= 3) {
				last_put_ = sbuff.getTime();
				async_impls_.put_back(impl_);
			}
			else
				thread_buffers_.put_front(pbuf, 0);

			return;
		}
		else
			async_impls_.put_back(impl_);

		if (size == 1)
			pbuf = new l_logbuffer;
		else
			pbuf = thread_buffers_.take_front();
	}

	impl_.buffer_ = pbuf;
	pbuf->append(sbuff.getData(), length);
	thread_buffers_.put_front(pbuf, 0);

	//wrk测试，qps下降幅度较大
	//async_buffers_.tryput_back(entry(&thread_buffers_, pbuf))
}

void asynclogger::threadFunc() {
	assert(running_);

	while (running_) {
		asyncimpl impl_ = async_impls_.take_front();
		l_logbuffer* pbuf = impl_.buffer_;

		if (pbuf) {
			impl_.output_->append(pbuf->getData(), pbuf->length());

			if (impl_.queue_->size() >= 4)
				delete pbuf;
			else {
				pbuf->reset();
				impl_.queue_->put_back(pbuf, 0);
			}

			impl_.output_->flush();
		}
	}
}

void asynclogger::stop() {
	running_ = 0;
	async_impls_.put_back(asyncimpl(0, 0, 0));
	thread_.join();
}
