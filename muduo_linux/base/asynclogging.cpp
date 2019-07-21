#include"asynclogging.h"
#include"logfile.h"

#include<assert.h>

asynclogging::asynclogging(const char* basename, off_t rollsize, int flush_interval)
	:basename_(basename),
	rollsize_(rollsize),
	flush_interval_(flush_interval),
	thread_(std::bind(&asynclogging::threadFunc,this)),
	buffer1_(new buffer),
	buffer2_(new buffer),
	buffers_(),
	running_(0) {

	buffer1_->bzero();
	buffer2_->bzero();
	buffers_.reserve(16);
}

void asynclogging::append(const char* data, size_t len) {
	klock<kmutex> x(&lock_);

	if (buffer1_->leftBytes() > len)
		buffer1_->append(data, len);
	else {
		buffers_.push_back(buffer1_);
		if (buffer2_) {
			buffer1_ = buffer2_;
			buffer2_ = nullptr;
		}
		else
			buffer1_ = new buffer;
		buffer1_->append(data, len);
		cond_.notify();
	}

}

void asynclogging::threadFunc() {
	assert(running_);
	
	logfile output_(basename_.c_str(), rollsize_);
	buffer* buffer3_(new buffer);
	buffer* buffer4_(new buffer);
	buffer3_->bzero();
	buffer4_->bzero();
	buffer_vec buffersw_;
	buffersw_.reserve(16);
	
	while (running_) {
		assert(buffer3_ && buffer3_->length() == 0);
		assert(buffer4_ && buffer4_->length() == 0);
		assert(buffersw_.empty());
		
		{
			klock<kmutex> x(&lock_);

			if (buffers_.empty())
				cond_.timedwait(&lock_, flush_interval_);

			buffers_.push_back(buffer1_);
			buffersw_.swap(buffers_);

			buffer1_ = buffer3_;
			buffer3_ = nullptr;
			if (!buffer2_) {
				buffer2_ = buffer4_;
				buffer4_ = nullptr;
			}
		}

		assert(!buffersw_.empty());

		for (size_t i = 0; i < buffersw_.size(); ++i) {
			output_.append(buffersw_[i]->getData(), buffersw_[i]->length());
			if (i > 1)
				delete buffersw_[i];
		}

		buffer3_ = buffersw_[0];
		buffer3_->reset();
		if (buffer4_ == nullptr) {
			buffer4_ = buffersw_[1];
			buffer4_->reset();
		}
		buffersw_.clear();

		output_.flush();
	}
	output_.flush();
	delete buffer3_;
	delete buffer4_;
}
