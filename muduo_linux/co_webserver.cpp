﻿#include"coloop.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"
#include"buffer.h"
#include"logging.h"

#include<unistd.h>
#include<fcntl.h>

struct connection {

	~connection() {
		delete sock_;
		delete cst_;
	}

	ksocket* sock_;
	coloop::coloop_item* cst_;
	
};

kmutex con_mutex;
std::vector<connection*> done_connections;
kmutex loop_mutex;
std::vector<coloop*> ioloops;
int thread_num = 4;

void httpCallback(const httprequest& request, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	if (request.getPath() == "/") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "text/plain");
		response.getBody() = "hello, world!\n";
		return;

		int fd = open("./html/index.html", O_RDONLY);
		char buf[1024];
		ssize_t nread = 0;
		while ((nread = read(fd, buf, 1024)) > 0)
			response.getBody().append(buf, nread);
		close(fd);
	}
	else {
		string path = "/home/rjingyu/html" + request.getPath();
		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0) {
			response.setStatu1(httpresponse::k404NotFound);
			response.setStatu2("Not Found");
			response.setKeepAlive(0);
		}
		else {
			response.setStatu1(httpresponse::k200OK);
			response.setStatu2("OK");

			size_t i = 0;
			string head = request.getHeader("Accept");
			while (i != head.size() && head[i] != ',')
				++i;
			response.addHeader("Content-Type", string(head.begin(), head.begin() + i));

			char buf[1024];
			ssize_t nread = 0;
			while ((nread = read(fd, buf, 1024)) > 0)
				response.getBody().append(buf, nread);
			close(fd);
		}
	}
}

void sendBuff(connection* con, buffer* buff) {
	ssize_t nwrote;
	int fd = con->cst_->getFd();
	while (1) {
		nwrote = write(fd, buff->beginPtr(), buff->usedBytes());
		if (nwrote < 0) {
			coroutine::yield();
			continue;
		}
		buff->retrieve(nwrote);

		if (buff->usedBytes()) {
			if (con->cst_->isWriting())
				coroutine::yield();
			else {
				con->cst_->disableReading();
				con->cst_->enableWriting();
				con->cst_->updateEvents();
			}
		}
		else
			break;
	}
	if (con->cst_->isWriting()) {
		con->cst_->disableWrting();
		con->cst_->enableReading();
		con->cst_->updateEvents();
	}
}

void connect_handler(connection* con) {
	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = con->cst_->getFd();

	while (1) {
		if ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || !request.praseRequest(&buff))
			break;
		else if (request.praseDone()) {
			LOG << "成功解析一次httprequest";
			string temp = request.getHeader("Connection");
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, response);

			buff.retrieve(request.getLength());
			request.reset();

			buffer buff2;
			response.appendToBuffer(&buff2);
			sendBuff(con, &buff2);
			LOG << "成功发送一次httpresponse";

			if (!response.keepAlive()) {
				con->sock_->shutdownWrite();
				break;
			}
		}
		coroutine::yield();
	}
	LOG << "连接处理完毕";
	klock<kmutex> x(&con_mutex);
	done_connections.push_back(con);
}

void accept_handler(connection* con) {
	sockaddr_in cliaddr;
	coloop* ioloop;
	int index = 0;
	int clifd;

	while (1) {
		clifd = con->sock_->accept(&cliaddr);
		if (clifd > 0) {
			connection* temp = new connection();

			temp->sock_ = new ksocket(clifd, cliaddr);
			temp->sock_->setTcpNodelay(1);

			ioloop = ioloops[index];
			index = (index + 1) % thread_num;

			temp->cst_ = new coloop::coloop_item(clifd, std::bind(connect_handler, temp), ioloop);
			temp->cst_->enableReading();
			temp->cst_->updateEvents();
			LOG << "建立一个新连接";
		}

		{
			klock<kmutex> x(&con_mutex);
			for (auto ptr : done_connections)
				delete ptr;
			done_connections.clear();
		}

		LOG << "清理connections";

		coroutine::yield();
	}
}

void thread_func() {
	coloop ioloop;
	{
		klock<kmutex> x(&loop_mutex);
		ioloops.push_back(&ioloop);
	}

	ioloop.loop();
}

int main(int argc, char* argv[]) {
	logger::createAsyncLogger();
	if (argc == 2) {
		thread_num = atoi(argv[1]);
		if (thread_num < 4)
			thread_num = 4;
		if (thread_num > 8)
			thread_num = 8;
	}

	std::vector<kthread*> thread_vec(thread_num);
	for (auto& x : thread_vec) {
		x = new kthread(thread_func);
		x->start();
	}

	coloop loop;
	connection con;

	ksocket sock("127.0.0.1", 7777);
	sock.bind();
	sock.listen();

	coloop::coloop_item cst(sock.getFd(), std::bind(accept_handler, &con), &loop);
	cst.enableReading();
	cst.updateEvents();

	con = { &sock,&cst };
	loop.loop();

	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}
