#include"logging.h"
#include"httpserver.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"

#include<unistd.h>
#include<fcntl.h>
#include<signal.h>

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

httpserver* pserver = nullptr;

void sigintHandler(int) {
	if (pserver)
		pserver->stop();
}

int main(int argc, char* argv[]) {
	if (argc != 4)
		return 0;
	
	signal(SIGINT, sigintHandler);
	signal(SIGPIPE, SIG_IGN);

	logger::createAsyncLogger();

	httpserver server(argv[1], atoi(argv[2]), atoi(argv[3]));
	pserver = &server;
	server.setHttpCallback(httpCallback);
	server.start();

	return 0;
}

/*
#include"coroutine.h"

void func2(coroutine_t* id) {
	printf("func2:1 %d\n", *id);
	coroutine* env = coroutine::threadEnv();
	env->yield();
	printf("func2:2 %d\n", *id);
}

void func1(coroutine_t* id) {
	printf("func1:1 %d\n", *id);
	coroutine* env = coroutine::threadEnv();
	coroutine_t child;
	child = env->create(std::bind(func2, id));
	env->resume(child);
	printf("func1:2 %d\n", *id);
	env->resume(child);
	env->free(child);
}

void func3() {
	coroutine* env = coroutine::threadEnv();
	coroutine_t id;
	for (int i = 0; i < 10; ++i) {
		id = env->create(std::bind(func1, &id));
		env->resume(id);
		env->free(id);
	}
	coroutine::freeEnv();
}
*/