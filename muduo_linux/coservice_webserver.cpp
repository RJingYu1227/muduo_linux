﻿#include"coservice.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"
#include"buffer.h"
#include"logging.h"

#include<unistd.h>
#include<signal.h>

void httpCallback(const httprequest& request, httpresponse& response);

coservice service;
thread_local std::vector<ksocket*> done_ksockets;

void sendBuff(buffer* buff) {
	ssize_t nwrote;
	coservice_item* cst = coservice_item::self();
	int fd = cst->getFd();
	while (1) {
		nwrote = write(fd, buff->beginPtr(), buff->usedBytes());
		if (nwrote < 0) {
			coroutine::yield();
			continue;
		}
		buff->retrieve(nwrote);

		if (buff->usedBytes()) {
			if (cst->isWriting())
				coroutine::yield();
			else {
				cst->disableReading();
				cst->enableWriting();

				cst->updateEvents();
			}
		}
		else
			break;
	}
	if (cst->isWriting()) {
		cst->disableWrting();
		cst->enableReading();

		cst->updateEvents();
	}
}

void connect_handler(ksocket* sock) {
	coservice_item* cst = coservice_item::self();
	cst->enableReading();
	cst->updateEvents();
	coservice::yield(6666);
	if (cst->getRevents() == 0) {
		LOG << "连接超时 " << sock->getAddr2() << ':' << sock->getPort();
		done_ksockets.push_back(sock);
		return;
	}

	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = cst->getFd();

	while (1) {
		if ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || !request.praseRequest(&buff))
			break;
		else if (request.praseDone()) {
			LOG << "完整解析httprequest " << sock->getAddr2() << ':' << sock->getPort();

			string temp = request.getHeader("Connection");
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, response);

			buffer buff2;
			response.appendToBuffer(&buff2);
			sendBuff(&buff2);
			LOG << "完整发送httpresponse " << sock->getAddr2() << ':' << sock->getPort();

			if (!response.keepAlive()) {
				sock->shutdownWrite();
				break;
			}
			else {
				buff.retrieve(request.getLength());
				request.reset();
			}
		}
		coroutine::yield();
	}

	LOG << "连接处理完毕 " << sock->getAddr2() << ':' << sock->getPort();
	done_ksockets.push_back(sock);
}

void accept_handler(ksocket* sock) {
	coservice_item* cst = coservice_item::self();
	cst->enableReading();
	cst->updateEvents();

	sockaddr_in cliaddr;
	int clifd;

	LOG << "TcpServer开始监听 " << sock->getAddr2() << ':' << sock->getPort();

	while (1) {
		clifd = sock->accept(&cliaddr);
		if (clifd > 0) {
			ksocket* temp_sock = new ksocket(clifd, cliaddr);
			temp_sock->setTcpNodelay(1);

			coservice_item::create(clifd, std::bind(connect_handler, temp_sock), &service);
			LOG << "建立一个新连接 " << temp_sock->getAddr2() << ':' << temp_sock->getPort();
		}

		for (auto temp : done_ksockets) {
			delete temp;
		}
		done_ksockets.clear();
		coroutine::yield();
	}

	LOG << "TcpServer停止监听 " << sock->getAddr2() << ':' << sock->getPort();
}

void thread_func() {
	service.run();
}

int main(int argc, char* argv[]) {
	logger::setFilename("./han.");
	logger::createAsyncLogger();
	signal(SIGPIPE, SIG_IGN);

	int thread_num = 4;
	if (argc == 2) {
		thread_num = atoi(argv[1]);
		if (thread_num < 4)
			thread_num = 4;
		if (thread_num > 8)
			thread_num = 8;
	}

	ksocket sock("0.0.0.0", 7777);
	sock.bind();
	sock.listen();

	coservice_item::create(sock.getFd(), std::bind(accept_handler, &sock), &service);

	std::vector<kthread*> thread_vec(thread_num);
	for (auto& x : thread_vec) {
		x = new kthread(thread_func);
		x->start();
	}
	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}