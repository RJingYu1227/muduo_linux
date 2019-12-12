#include"coloop.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"buffer.h"
#include"logging.h"

#include<unistd.h>
#include<signal.h>

kmutex loop_mutex;
std::vector<coloop*> ioloops;
int thread_num = 4;

void httpCallback(const httprequest& request, const string& content, httpresponse& response);

void sendBuff(buffer* buff) {
	ssize_t nwrote;
	coloop_item* cpt = coloop_item::self();
	int fd = cpt->getFd();
	while (1) {
		nwrote = write(fd, buff->beginPtr(), buff->usedBytes());
		if (nwrote < 0) {
			coroutine::yield();
			continue;
		}
		buff->retrieve(nwrote);

		if (buff->usedBytes()) {
			if (cpt->isWriting() == false) {
				cpt->disableReading();
				cpt->enableWriting();
				cpt->updateEvents();
			}

			coroutine::yield();
		}
		else
			break;
	}
	if (cpt->isWriting()) {
		cpt->disableWrting();
		cpt->enableReading();
		cpt->updateEvents();
	}
}

void connect_handler() {
	coloop_item* cpt = coloop_item::self();
	cpt->enableReading();
	cpt->updateEvents();

	LOG << "处理新连接 " << cpt->getAddr2() << ':' << cpt->getPort();
	coroutine::yield();

	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = cpt->getFd();

	while (1) {
		if ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || request.parseRequest(&buff) == false)
			break;
		else if (request.parseDone()) {
			LOG << "完整解析httprequest " << cpt->getAddr2() << ':' << cpt->getPort();

			string temp = request.getHeader("Connection");
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, buff.toString(), response);

			buffer buff2;
			response.appendToBuffer(&buff2);
			sendBuff(&buff2);
			LOG << "完整发送httpresponse " << cpt->getAddr2() << ':' << cpt->getPort();

			if (!response.keepAlive()) {
				cpt->shutdownWrite();
				break;
			}
			else {
				buff.retrieve(request.getLength());
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

	while (1) {
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
		klock<kmutex> x(&loop_mutex);
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

	std::vector<kthread*> thread_vec(thread_num);
	for (auto& x : thread_vec) {
		x = new kthread(thread_func);
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
