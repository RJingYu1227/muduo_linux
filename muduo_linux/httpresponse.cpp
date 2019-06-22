#include"httpresponse.h"
#include<string.h>

void httpresponse::appendToBuffer(buffer* buffer2) {
	char buf[32];

	snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statu1_);
	buffer2->append(buf, strlen(buf));
	buffer2->append(statu2_);

	if (alive_)
		buffer2->append("Connection: keep-alive\r\n");
	else
		buffer2->append("Connection: close\r\n");

	snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
	buffer2->append(buf, strlen(buf));

	for (auto iter : headers_)
		buffer2->append(iter.first + iter.second);

	buffer2->append("\r\n", 2);
	buffer2->append(body_);

}