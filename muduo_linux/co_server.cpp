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

	ssize_t nread;
	char buf[1024];
	while (1) {
		nread = read(sokt->getFd(), buf, 1024);
		if (nread <= 0)
			break;
		write(sokt->getFd(), msg.c_str(), msg.size());

		loop->runAfter(6666, env->self());
		env->yield();
	}
	printf("客户端关闭连接或者超时\n");

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
			loop->runAfter(6666, id);
			continue;
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
	loop->add(socket_.getFd(), coloop::READ | coloop::ET, id);
	loop->loop();
	loop->remove(socket_.getFd());

	coloop::freeColoop();
	coroutine::freeCoenv();
}
*/