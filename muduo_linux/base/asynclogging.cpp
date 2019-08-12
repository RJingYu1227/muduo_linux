#include"asynclogging.h"
#include"logfile.h"

#include<assert.h>

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize)
	:basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogger::threadFunc, this)),
	running_(0) {

}

asynclogger::~asynclogger() {
	if (running_) {
		running_ = 0;
		async_impls_.put_back(impl(0, 0, 0));
		thread_.join();
	}
}

void asynclogger::append(const s_logbuffer& sbuff) {
	/*
	这两种实现方式的转变对qps的影响不大
	thread_local buffer_queue thread_buffers;
	thread_local logfile output((basename_ + std::to_string(pthread_self()) + '.').c_str(), rollsize_);
	thread_local impl impl_(nullptr, &thread_buffers_, &output_);
	*/

	/*
	wrk测试，qps下降幅度较大
	async_buffers_.tryput_back(entry(&thread_buffers_, pbuf));
	*/

	//得想个办法去掉thread_local
	thread_local time_t last_put = ::time(NULL);

	impl* pimpl_ = thread_pimpl_.get();
	if (pimpl_ == nullptr) {
		impl* temp = new impl(
			nullptr,
			new buffer_queue(),
			new logfile((basename_ + std::to_string(pthread_self()) + '.').c_str(), rollsize_));

		thread_pimpl_.set(temp);
		pimpl_ = temp;
	}

	l_logbuffer* pbuf = nullptr;
	size_t size = pimpl_->queue_->size();
	size_t length = sbuff.length();

	if (size == 0)
		pbuf = new l_logbuffer;
	else {
		pbuf = pimpl_->queue_->take_front();
		pimpl_->buffer_ = pbuf;

		if (pbuf->leftBytes() >= length) {
			pbuf->append(sbuff.getData(), length);

			if (sbuff.getTime() - last_put >= 3) {
				last_put = sbuff.getTime();
				async_impls_.put_back(*pimpl_);
			}
			else
				pimpl_->queue_->put_front(pbuf, 0);

			return;
		}
		else
			async_impls_.put_back(*pimpl_);

		if (size == 1)
			pbuf = new l_logbuffer;
		else
			pbuf = pimpl_->queue_->take_front();
	}

	pimpl_->buffer_ = pbuf;
	pbuf->append(sbuff.getData(), length);
	pimpl_->queue_->put_front(pbuf, 0);
}

void asynclogger::threadFunc() {
	assert(running_);

	while (running_) {
		impl impl_ = async_impls_.take_front();
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