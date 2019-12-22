#include"httprequest.h"
#include"httpresponse.h"

#include<pax/coroutine/coloop.h>
#include<pax/base/buffer.h>
#include<pax/log/logging.h>
#include<pax/base/thread.h>

#include<unistd.h>
#include<signal.h>

using namespace::pax;

mutex loop_mutex;
std::vector<coloop*> ioloops;
int thread_num = 4;

void httpCallback(const httprequest& request, const string& content, httpresponse& response);

void connect_handler() {
	coloop_item* cpt = coloop_item::self();
	cpt->enableReading();
	cpt->updateEvents();

	LOG << "处理新连接 " << cpt->getAddr2() << ':' << cpt->getPort();
	coroutine::yield();

	ssize_t nread;
	buffer buff1, buff2;
	httprequest request;

	for (;;) {
		if ((nread = cpt->read(buff1.endPtr(), 1024)) > 0) {
			buff1.hasUsed(nread);
			buff1.ensureLeftBytes(1024);
		}

		if (nread == 0 || request.parseRequest(buff1) == false)
			break;
		else if (request.parseDone()) {
			LOG << "完整解析httprequest " << cpt->getAddr2() << ':' << cpt->getPort();

			string temp;
			try {
				temp = request.getHeader("Connection");
			}
			catch (std::runtime_error er) {
				temp.clear();
			}
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, buff1.toString(), response);

			response.appendToBuffer(buff2);
			cpt->write(buff2.beginPtr(), buff2.usedBytes(), -1);
			LOG << "完整发送httpresponse " << cpt->getAddr2() << ':' << cpt->getPort();

			if (!response.keepAlive()) {
				cpt->shutdownWrite();
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

	LOG << "连接处理完毕 " << cpt->getAddr2() << ':' << cpt->getPort();
}

void accept_handler() {
	coloop_item* cpt = coloop_item::self();
	cpt->bind();
	cpt->listen();
	cpt->enableReading();
	cpt->updateEvents();

	LOG << "TcpServer开始监听 " << cpt->getAddr2() << ':' << cpt->getPort();
	coroutine::yield();

	sockaddr_in cliaddr;
	int clifd;
	coloop* ioloop;
	int index = 0;

	for (;;) {
		clifd = cpt->accept(&cliaddr);
		if (clifd > 0) {
			ioloop = ioloops[index];
			index = (index + 1) % thread_num;

			coloop_item::create(connect_handler, clifd, cliaddr, ioloop);
		}

		coroutine::yield();
	}

	LOG << "TcpServer停止监听 " << cpt->getAddr2() << ':' << cpt->getPort();
}

void thread_func() {
	coloop ioloop;
	{
		lock<mutex> x(&loop_mutex);
		ioloops.push_back(&ioloop);
	}

	ioloop.loop();
}

int main(int argc, char* argv[]) {
	logger::createAsyncLogger();
	signal(SIGPIPE, SIG_IGN);

	if (argc == 2) {
		thread_num = atoi(argv[1]);
		if (thread_num < 4)
			thread_num = 4;
		if (thread_num > 8)
			thread_num = 8;
	}

	std::vector<thread*> thread_vec(thread_num);
	for (auto& x : thread_vec) {
		x = new thread(thread_func);
		x->start();
	}

	coloop loop;
	coloop_item::create(accept_handler, "0.0.0.0", 7777, &loop);
	loop.loop();

	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}
