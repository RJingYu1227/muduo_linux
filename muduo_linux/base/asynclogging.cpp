#include"asynclogging.h"
#include"logfile.h"

#include<assert.h>

thread_local asynclogging::buffer_queue asynclogging::thread_buffers_;
thread_local time_t asynclogging::last_put_ = time(NULL);
blockqueue<asynclogging::entry> asynclogging::async_buffers_;

asynclogging::asynclogging(const char* basename, off_t rollsize)
	:basename_(basename),
	rollsize_(rollsize),
	thread_(std::bind(&asynclogging::threadFunc, this)),
	running_(0) {

}

void asynclogging::append(const char* data, size_t len, time_t time) {
	buffer* pbuf = nullptr;
	size_t size = thread_buffers_.size();

	if (size == 0)
		pbuf = new buffer;
	else {
		pbuf = thread_buffers_.take_front();
		if (pbuf->leftBytes() >= len) {
			pbuf->append(data, len);
			if (time - last_put_ >= 3) {
				last_put_ = time;
				async_buffers_.put_back(entry(&thread_buffers_, pbuf));
			}
			else
				thread_buffers_.put_front(pbuf, 0);

			return;
		}
		else
			async_buffers_.put_back(entry(&thread_buffers_, pbuf));

		if (size == 1)
			pbuf = new buffer;
		else
			pbuf = thread_buffers_.take_front();
	}

	pbuf->append(data, len);
	thread_buffers_.put_front(pbuf, 0);

	//wrk测试，qps下降幅度较大
	//async_buffers_.tryput_back(entry(&thread_buffers_, pbuf))
}

void asynclogging::threadFunc() {
	assert(running_);

	logfile output_(basename_.c_str(), rollsize_);

	while (running_) {
		entry temp = async_buffers_.take_front();
		buffer* pbuf = temp.second;
		if (pbuf) {
			output_.append(pbuf->getData(), pbuf->length());
			if (temp.first->size() >= 4)
				delete pbuf;
			else {
				pbuf->reset();
				temp.first->put_back(pbuf, 0);
			}
		}

		output_.flush();
	}
}

void asynclogging::stop() {
	running_ = 0;
	async_buffers_.put_back(entry(nullptr, nullptr));
	thread_.join();
}
