#include"httprequest.h"
#include"httpresponse.h"

#include<pax/coroutine/coservice.h>
#include<pax/base/buffer.h>
#include<pax/log/logging.h>
#include<pax/base/thread.h>

#include<unistd.h>
#include<signal.h>

using namespace::pax;

void httpCallback(const httprequest& request, const string& content, httpresponse& response);

coservice service;

void connect_handler() {
	coservice_item* cst = coservice_item::self();
	cst->enableReading();
	cst->updateEvents();

	LOG << "处理新连接 " << cst->getIp() << ':' << cst->getPort();
	coroutine::yield();

	buffer buff1, buff2;
	httprequest request;

	for (;;) {
		ssize_t nread = cst->read(buff1.endPtr(), 1024);
		if (nread > 0) {
			buff1.hasUsed(nread);
			buff1.ensureLeftBytes(1024);
		}

		if (nread == 0 || request.parseRequest(buff1) == false)
			break;
		else if (request.parseDone()) {
			LOG << "完整解析httprequest " << cst->getIp() << ':' << cst->getPort();
			
			string temp;
			try {
				temp = request.getHeader("Connection");
			}
			catch (std::runtime_error& er) {
				temp.clear();
			}
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, buff1.toString(), response);

			response.appendToBuffer(buff2);
			cst->write(buff2.beginPtr(), buff2.usedBytes(), -1);
			LOG << "完整发送httpresponse " << cst->getIp() << ':' << cst->getPort();

			if (!response.keepAlive()) {
				cst->shutdownWrite();
				break;
			}
			else {
				buff1.retrieve(request.getLength());
				buff2.retrieveAll();
				request.reset();
			}
		}
		coroutine::yield();
	}

	LOG << "连接处理完毕 " << cst->getIp() << ':' << cst->getPort();
}

void accept_handler() {
	coservice_item* cst = coservice_item::self();
	cst->bind("0.0.0.0", 7777, 1);
	cst->listen();
	cst->enableReading();
	cst->updateEvents();

	LOG << "TcpServer开始监听 " << cst->getIp() << ':' << cst->getPort();
	coroutine::yield();
	
	sockaddr_in cliaddr;

	for (;;) {
		int clifd = cst->accept(cliaddr);
		if (clifd > 0)
			coservice_item::create(connect_handler, clifd, cliaddr, &service);

		coroutine::yield();
	}

	LOG << "TcpServer停止监听 " << cst->getIp() << ':' << cst->getPort();
}

void thread_func() {
	service.run();
}

int main(int argc, char* argv[]) {
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

	coservice_item::create(accept_handler, &service);

	std::vector<thread*> thread_vec(thread_num);
	for (auto& x : thread_vec) {
		x = new thread(thread_func);
		x->start();
	}
	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}