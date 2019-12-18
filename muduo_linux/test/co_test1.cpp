#include"coroutine.h"

#include<string>
#include<iostream>

using namespace::std;

int main() {
	string ch;

	auto push = [&](const char* str) {
		ch = str;
		coroutine::yield();
	};

	coroutine_item push_co([&]() {
		push("001");
		push("010");
		push("011");
	});

	auto pull = [&]() {
		cout << ch << endl;
		push_co.resume();
	};

	push_co.resume();
	pull();
	pull();
	pull();

	return 0;
}