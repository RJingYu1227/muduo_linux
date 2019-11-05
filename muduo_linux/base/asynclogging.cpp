#include"asynclogging.h"
#include"logfile.h"

asynclogger* asynclogger::instance_ = nullptr;

asynclogger::asynclogger(const char* basename, off_t rollsize) 
	:thread_pimpl_(nullptr),
	basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogger::threadFunc, this)),
	running_(0) {

}

asynclogger::~asynclogger() {
	if (running_) {
		running_ = 0;
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

	impl* pimpl = thread_pimpl_.get();
	if (pimpl == nullptr) {
		pimpl = new impl(
			new l_logbuffer(),
			new buffer_queue(),
			new logfile((basename_ + std::to_string(pthread_self()) + '.').c_str(), rollsize_));
		thread_pimpl_.set(pimpl);

		pimpl->buffer_->append(sbuff.getData(), sbuff.length());
		pimpl->queue_->put_front(pimpl->buffer_);

		pimpl->buffer_ = new l_logbuffer();
		klock<kmutex> x(&lock_);
		empty_impls_.push_back(*pimpl);

		return;
	}

	l_logbuffer* pbuf = nullptr;
	size_t size = pimpl->queue_->size();

	if (size == 0)
		pbuf = new l_logbuffer();
	else {
		pbuf = pimpl->queue_->take_front();

		if (pbuf->leftBytes() >= sbuff.length()) {
			pbuf->append(sbuff.getData(), sbuff.length());
			pimpl->queue_->put_front(pbuf);

			return;
		}
		else
			full_impls_.put_back(impl(pbuf, pimpl->queue_, pimpl->output_));//注意这里

		if (size == 1)
			pbuf = new l_logbuffer();
		else
			pbuf = pimpl->queue_->take_front();
	}

	pbuf->append(sbuff.getData(), sbuff.length());
	pimpl->queue_->put_front(pbuf);
}

void asynclogger::threadFunc() {
	impl impl_;
	l_logbuffer* pbuf;
	size_t fsize;

	while (running_) {
		fsize = full_impls_.timedwait(6);
		if (fsize) {
			while (fsize--) {
				impl_ = full_impls_.take_front();
				pbuf = impl_.buffer_;
				impl_.output_->append(pbuf->getData(), pbuf->length());
				impl_.output_->flush();

				if (impl_.queue_->size() >= 4)
					delete pbuf;
				else {
					pbuf->reset();
					impl_.queue_->put_back(pbuf);
				}
			}
		}
		else {
			klock<kmutex> x(&lock_);
			for (auto& temp : empty_impls_) {
				temp.queue_->put_back(temp.buffer_);
				temp.buffer_ = temp.queue_->take_front();

				if (temp.buffer_->length()) {
					temp.output_->append(temp.buffer_->getData(), temp.buffer_->length());
					temp.output_->flush();
					temp.buffer_->reset();
				}
			}
		}
	}
}