
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

struct connection {

	~connection() {
		delete sock_;
		delete cpt_;
	}

	ksocket* sock_;
	coloop::coloop_item* cpt_;

};

void connect_handler(connection* con) {
	ssize_t nread;
	char buf[1024];
	while (1) {
		if (con->cpt_->getRevents()) {
			nread = read(con->sock_->getFd(), &buf, 1024);
			if (nread == 0)
				break;

			write(con->sock_->getFd(), msg.c_str(), msg.size());
			con->cpt_->setTimeout(6666);
			coroutine::yield();
		}
		else
			break;
	}
	printf("客户端关闭连接或超时\n");
}

void accept_handler(connection* con) {
	sockaddr_in cliaddr;
	int clifd;
	coloop::coloop_item* cpt;

	while (1) {
		clifd = con->sock_->accept(&cliaddr);
		if (clifd > 0) {
			connection* temp = new connection();
			cpt = new coloop::coloop_item(clifd, std::bind(connect_handler, temp));

			temp->sock_ = new ksocket(clifd, cliaddr);
			temp->cpt_ = cpt;
			temp->cpt_->enableReading();
			temp->cpt_->updateEvents();

			continue;
		}
		coroutine::yield();
	}
}

int main() {
	ksocket sock("127.0.0.1", 7777);
	sock.bind();
	sock.listen();

	connection con;
	coloop::coloop_item cpt(sock.getFd(), std::bind(accept_handler, &con));
	cpt.enableReading();
	cpt.enableEpollet();
	cpt.updateEvents();

	con = { &sock,&cpt };
	coloop::loop();

	return 0;
}
