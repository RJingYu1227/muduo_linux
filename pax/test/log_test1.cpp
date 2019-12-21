#include<pax/log/logging.h>

using namespace::pax;

int main() {
	LOG << "test1";
	logger() << "test2";
	logger() << LOG_HEAD << "test3";

	logger log;
	log << LOG_HEAD;
	log << "test4";
	log << logger::time << logger::flush;
	log << logger::error << '\n';

	logger log2 = log;

	return 0;
}