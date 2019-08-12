﻿#include"logging.h"
#include"httpserver.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"

#include<unistd.h>
#include<fcntl.h>

void httpCallback(const httprequest& request, httpresponse& response) {
	if (request.getPath() == "/") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "text/html");
		response.addHeader("Server", "RJingYu");
		string now = ktimer::timeToString(ktimer::getUnixTime());
		response.getBody() = "<html><head><title>This is title</title></head>"
			"<body><h1>Hello</h1>Now is " + now +
			"</body></html>";
	}
	else if (request.getPath() == "/yujing") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "image/png");
		response.addHeader("Server", "RJingYu");

		//注意文件权限
		//png文件为大端序
		int fd = open("/home/rjingyu/yujing.png", O_RDONLY);
		char buf[64 * 1024];
		ssize_t nread = 0;
		while ((nread = read(fd, buf, 64 * 1024)) > 0)
			response.getBody().append(buf, buf + nread);
		close(fd);
	}
	else if (request.getPath() == "/hello") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "text/plain");
		response.addHeader("Server", "RJingYu");
		response.getBody() = "hello, world!\n";
	}
	else {
		response.setStatu1(httpresponse::k404NotFound);
		response.setStatu2("Not Found");
		response.setKeepAlive(0);
	}
}

int main(int argc, char* argv[]) {
	if (argc != 4)
		return 0;

	logger::createAsyncLogger();
	tcpconnection::ignoreSigPipe();

	httpserver server(argv[1], atoi(argv[2]), atoi(argv[3]));
	server.setHttpCallback(httpCallback);
	server.start();

	return 0;
}