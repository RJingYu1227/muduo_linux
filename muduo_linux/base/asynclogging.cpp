#include"asynclogging.h"
#include"logfile.h"

#include<assert.h>

blockqueue<asynclogging::entry> asynclogging::async_queue_;
thread_local asynclogging::buffer_queue asynclogging::thread_buffers_;

asynclogging::asynclogging(const char* basename, off_t rollsize, int flush_interval)
	:basename_(basename),
	rollsize_(rollsize),
	flush_interval_(flush_interval),
	thread_(std::bind(&asynclogging::threadFunc, this)),
	running_(0) {

}

void asynclogging::append(const char* data, size_t len) {
	buffer* pbuf = nullptr;

	while (!thread_buffers_.empty()) {
		pbuf = thread_buffers_.take_front();
		if (pbuf->leftBytes() >= len) {
			pbuf->append(data, len);
			break;
		}
		else {
			async_queue_.put_back(entry(&thread_buffers_, pbuf));
			pbuf = nullptr;
		}
	}

	if (pbuf == nullptr) {
		pbuf = new buffer;
		pbuf->append(data, len);
	}

	thread_buffers_.put_front(pbuf);
}

void asynclogging::threadFunc() {
	assert(running_);

	logfile output_(basename_.c_str(), rollsize_);

	while (running_) {
		entry temp = async_queue_.take_front();
		buffer* pbuf = temp.second;
		if (pbuf) {
			output_.append(pbuf->getData(), pbuf->length());
			if (temp.first->size() < 4) {
				pbuf->reset();
				temp.first->put_back(pbuf);
			}
			else
				delete pbuf;
		}

		output_.flush();
	}
}

void asynclogging::stop() {
	running_ = 0;
	async_queue_.put_back(entry(nullptr, nullptr));
	thread_.join();
}
