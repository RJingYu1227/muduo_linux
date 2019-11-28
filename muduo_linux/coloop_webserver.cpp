#include"coloop.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"buffer.h"
#include"logging.h"

#include<unistd.h>
#include<signal.h>

kmutex sock_mutex;
std::vector<ksocket*> done_ksockets;
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

void connect_handler(ksocket* sock) {
	coloop_item* cpt = coloop_item::self();
	cpt->enableReading();
	cpt->updateEvents();
	coloop::yield(6666);
	if (cpt->getRevents() == 0) {
		LOG << "连接超时 " << sock->getAddr2() << ':' << sock->getPort();
		klock<kmutex> x(&sock_mutex);
		done_ksockets.push_back(sock);

		return;
	}

	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = cpt->getFd();

	while (1) {
		if ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || request.praseRequest(&buff) == false)
			break;
		else if (request.praseDone()) {
			LOG << "完整解析httprequest " << sock->getAddr2() << ':' << sock->getPort();

			string temp = request.getHeader("Connection");
			bool alive = (temp == "keep-alive") ||
				(request.getVersion() == httprequest::kHTTP11 && temp != "close");

			httpresponse response(alive);
			httpCallback(request, buff.toString(), response);

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
	klock<kmutex> x(&sock_mutex);
	done_ksockets.push_back(sock);
}

void accept_handler(ksocket* sock) {
	coloop_item* cpt = coloop_item::self();
	cpt->enableReading();
	cpt->updateEvents();
	coloop::yield(6666);

	sockaddr_in cliaddr;
	int clifd;
	coloop* ioloop;
	int index = 0;

	LOG << "TcpServer开始监听 " << sock->getAddr2() << ':' << sock->getPort();

	while (1) {
		clifd = sock->accept(&cliaddr);
		if (clifd > 0) {
			ksocket* temp_sock = new ksocket(clifd, cliaddr);
			temp_sock->setTcpNodelay(1);

			ioloop = ioloops[index];
			index = (index + 1) % thread_num;

			coloop_item::create(clifd, std::bind(connect_handler, temp_sock), ioloop);
			LOG << "建立一个新连接 " << temp_sock->getAddr2() << ':' << temp_sock->getPort();
		}

		{
			klock<kmutex> x(&sock_mutex);
			for (auto ptr : done_ksockets)
				delete ptr;
			done_ksockets.clear();
		}

		coroutine::yield();
	}

	LOG << "TcpServer停止监听 " << sock->getAddr2() << ':' << sock->getPort();
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
	ksocket sock("127.0.0.1", 7777);
	sock.bind();
	sock.listen();

	coloop_item::create(sock.getFd(), std::bind(accept_handler, &sock), &loop);
	loop.loop();

	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}
