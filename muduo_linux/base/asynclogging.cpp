#include"asynclogging.h"
#include"logfile.h"

#include<assert.h>

asynclogging::asynclogging(const char* basename, off_t rollsize, int flush_interval)
	:basename_(basename),
	rollsize_(rollsize),
	flush_interval_(flush_interval),
	thread_(std::bind(&asynclogging::threadFunc, this)),
	running_(0) {

}

void asynclogging::append(const char* data, size_t len) {
	static thread_local buffer_queue thread_buffers_;

	buffer* pbuf = nullptr;
	vector<entry> vec;
	while (!thread_buffers_.empty()) {
		pbuf = thread_buffers_.take();
		if (pbuf->leftBytes() >= len) {
			pbuf->append(data, len);
			break;
		}
		else {
			vec.push_back(entry(&thread_buffers_, pbuf));
			pbuf = nullptr;
		}
	}

	if (!vec.empty())
		async_buffers_.put(vec);

	if (pbuf == nullptr) {
		pbuf = new buffer;
		pbuf->append(data, len);
	}
	thread_buffers_.put(pbuf);
}

void asynclogging::threadFunc() {
	assert(running_);

	logfile output_(basename_.c_str(), rollsize_);

	while (running_) {
		entry temp = async_buffers_.take();
		buffer* pbuf = temp.second;
		output_.append(pbuf->getData(), pbuf->length());
		if (temp.first->size() < 4) {
			pbuf->reset();
			temp.first->put(pbuf);
		}
		else
			delete pbuf;

		output_.flush();
	}
}
