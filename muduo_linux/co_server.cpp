#include"coloop.h"
#include"ksocket.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"
#include"buffer.h"

#include<unistd.h>
#include<fcntl.h>

struct connection {

	~connection() {
		delete sock_;
		delete cpt_;
	}

	ksocket* sock_;
	coloop::coloop_item* cpt_;

};

thread_local std::vector<connection*> free_conmap;

void httpCallback(const httprequest& request, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	if (request.getPath() == "/") {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.addHeader("Content-Type", "text/html");

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
	int fd = con->cpt_->getFd();
	while (1) {
		nwrote = write(fd, buff->beginPtr(), buff->usedBytes());
		if (nwrote < 0) {
			coroutine::yield();
			continue;
		}
		buff->retrieve(nwrote);

		if (buff->usedBytes()) {
			if (con->cpt_->isWriting())
				coroutine::yield();
			else {
				con->cpt_->disableReading();
				con->cpt_->enableWriting();
				con->cpt_->updateEvents();
			}
		}
		else
			break;
	}
	if (con->cpt_->isWriting()) {
		con->cpt_->disableWrting();
		con->cpt_->enableReading();
		con->cpt_->updateEvents();
	}
}

void connect_handler(connection* con) {
	ssize_t nread;
	buffer buff;
	httprequest request;
	int fd = con->cpt_->getFd();

	while (1) {
		while ((nread = read(fd, buff.endPtr(), 1024)) > 0) {
			buff.hasUsed(nread);
			buff.ensureLeftBytes(1024);
		}

		if (nread == 0 || !request.praseRequest(&buff))
			break;
		else if (request.praseDone()) {
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

			if (!response.keepAlive()) {
				con->sock_->shutdownWrite();
				break;
			}
		}
		coroutine::yield();
	}
	free_conmap.push_back(con);
}

void accept_handler(connection* con) {
	sockaddr_in cliaddr;
	int clifd;

	while (1) {
		clifd = con->sock_->accept(&cliaddr);
		if (clifd > 0) {
			connection* temp = new connection();

			temp->sock_ = new ksocket(clifd, cliaddr);
			temp->sock_->setTcpNodelay(1);

			temp->cpt_ = new coloop::coloop_item(clifd, std::bind(connect_handler, temp));
			temp->cpt_->enableReading();
			temp->cpt_->enableEpollet();
			temp->cpt_->updateEvents();

			continue;
		}

		for (auto ptr : free_conmap)
			delete ptr;
		free_conmap.clear();

		coroutine::yield();
	}
}

void thread_func() {
	connection con;

	ksocket sock("127.0.0.1", 7777);
	sock.setReuseAddr(1);
	sock.setReusePort(1);
	sock.bind();
	sock.listen();

	coloop::coloop_item cpt(sock.getFd(), std::bind(accept_handler, &con));
	cpt.enableReading();
	cpt.enableEpollet();
	cpt.updateEvents();

	con = { &sock,&cpt };
	coloop::loop();
}

int main(int argc, char* argv[]) {
	int thread_num = 4;
	if (argc == 1) {
		thread_num = atoi(argv[0]);
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
	for (auto& x : thread_vec)
		x->join();
	for (auto& x : thread_vec)
		delete x;

	return 0;
}
