#include "fileutil.h"

#include<assert.h>

appendfile::appendfile(const char* filename)
	:fp_(fopen(filename, "ae")),
	written_bytes_(0) {

	assert(fp_);
	setbuffer(fp_, buffer_, sizeof buffer_);
}

appendfile::~appendfile() {
	fclose(fp_);
}

void appendfile::append(const char* data, size_t len) {
	size_t remain = len;
	while (remain > 0) {
		size_t n = fwrite_unlocked(data, 1, len, fp_);
		if (n == 0) {
			int err = ferror(fp_);
			//对同一个文件,每一次调用输入输出函数，均产生一个新的ferror函数值
			if (err)
				fprintf(stderr, "appendfile::append() failed !\n");
			break;
		}
		remain -= n;
	}
	written_bytes_ += len;
}

void appendfile::flush() {
	fflush(fp_);
}