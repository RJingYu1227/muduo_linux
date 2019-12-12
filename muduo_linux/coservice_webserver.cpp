#include"coservice.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"buffer.h"
#include"logging.h"

#include<unistd.h>
#include<signal.h>

void httpCallback(const httprequest& request, const string& content, httpresponse& response);

coservice service;

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
			if (cst->isWriting() == false) {
				cst->disableReading();
				cst->enableWriting();

				cst->updateEvents();
			}

			coroutine::yield();
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

void connect_handler() {
	coservice_item* cst = coservice_item::self();
	cst->enableReading();
	cst->updateEvents();

	LOG << "处理新连接 " << cst->getAddr2() << ':' << cst->getPort();
	coroutine::yield();

	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = cst->getFd();

	while (1) {
		if ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || request.parseRequest(&buff) == false)
			break;
		else if (request.parseDone()) {
			LOG << "完整解析httprequest " << cst->getAddr2() << ':' << cst->getPort();

			string temp = request.getHeader("Connection");
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, buff.toString(), response);

			buffer buff2;
			response.appendToBuffer(&buff2);
			sendBuff(&buff2);
			LOG << "完整发送httpresponse " << cst->getAddr2() << ':' << cst->getPort();

			if (!response.keepAlive()) {
				cst->shutdownWrite();
				break;
			}
			else {
				buff.retrieve(request.getLength());
				request.reset();
			}
		}
		coroutine::yield();
	}

	LOG << "连接处理完毕 " << cst->getAddr2() << ':' << cst->getPort();
}

void accept_handler() {
	coservice_item* cst = coservice_item::self();
	cst->bind();
	cst->listen();
	cst->enableReading();
	cst->updateEvents();

	LOG << "TcpServer开始监听 " << cst->getAddr2() << ':' << cst->getPort();
	coroutine::yield();
	
	sockaddr_in cliaddr;
	int clifd;

	while (1) {
		clifd = cst->accept(&cliaddr);
		if (clifd > 0)
			coservice_item::create(connect_handler, clifd, cliaddr, &service);

		coroutine::yield();
	}

	LOG << "TcpServer停止监听 " << cst->getAddr2() << ':' << cst->getPort();
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

	coservice_item::create(accept_handler, "0.0.0.0", 7777, &service);

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