#include<pax/log/fileutil.h>

#include<assert.h>

using namespace::pax;

appendfile::appendfile(const char* filename)
	:fp_(fopen(filename, "ae")),
	written_bytes_(0) {

	assert(fp_);
	setbuffer(fp_, buffer_, sizeof buffer_);
}

void appendfile::append(const char* data, size_t len) {
	size_t remain = len;
	while (remain) {
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
