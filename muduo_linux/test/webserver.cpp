#include"httpserver.h"

#include"log/logging.h"

#include<signal.h>

using namespace::pax;

httpserver* pserver = nullptr;

void sigintHandler(int) {
	if (pserver)
		pserver->stop();
}

int main(int argc, char* argv[]) {
	if (argc != 4)
		return 0;

	::signal(SIGINT, sigintHandler);
	::signal(SIGPIPE, SIG_IGN);

	logger::createAsyncLogger();

	httpserver server(argv[1], atoi(argv[2]), atoi(argv[3]));
	pserver = &server;
	server.start();

	return 0;
}