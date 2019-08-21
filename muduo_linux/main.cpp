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
#include"coloop.h"
#include"ksocket.h"

#include<unistd.h>
#include<string>

std::string msg =
"HTTP/1.1 200 OK\r\n"
"Connection: keep-alive\r\n"
"Content-Length: 13\r\n"
"Content-Type: text/plain\r\n"
"Server: RJingYu\r\n"
"\r\n"
"hello, world\n";

void connection(ksocket* sokt) {
	coroutine* env = coroutine::threadCoenv();
	coloop* loop = coloop::threadColoop();

	char buf[1024];
	while (1) {
		ssize_t nread = read(sokt->getFd(), buf, 1024);
		if (nread <= 0)
			break;
		write(sokt->getFd(), msg.c_str(), msg.size());

		env->yield();
	}
	loop->remove(sokt->getFd(), coloop::READ, env->self());

	delete sokt;
}

void server(ksocket* sokt) {
	coroutine* env = coroutine::threadCoenv();
	coloop* loop = coloop::threadColoop();

	sockaddr_in cliaddr;
	int clifd;
	coroutine_t id;

	while (1) {
		clifd = sokt->accept(&cliaddr);
		if (clifd > 0) {
			ksocket* cskt = new ksocket(clifd, cliaddr);
			id = env->create(std::bind(connection, cskt));
			loop->add(clifd, coloop::READ, id);
		}
		env->yield();
	}
}

int main() {
	coroutine* env = coroutine::threadCoenv();
	coloop* loop = coloop::threadColoop();

	ksocket socket_("127.0.0.1", 6666);
	socket_.bind();
	socket_.listen();

	coroutine_t id = env->create(std::bind(server, &socket_));
	loop->add(socket_.getFd(), coloop::READ, id);

	loop->loop(-1);

	coloop::freeColoop();
	coroutine::freeCoenv();
}
*/