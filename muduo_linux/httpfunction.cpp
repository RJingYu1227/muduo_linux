#include"httprequest.h"
#include"httpresponse.h"

#include<unistd.h>
#include<fcntl.h>
#include<unordered_map>
#include<string>

using namespace::std;

thread_local unordered_map<string, string> redis;

bool insertRedis(const string& path) {
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		return 0;

	char buf[1024];
	ssize_t nread = 0;
	redis[path] = "";
	string& value = redis[path];

	while ((nread = read(fd, &buf, 1024)) > 0)
		value.append(buf, nread);

	close(fd);
	return 1;
}

void httpCallback(const httprequest& request, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	bool ok = 0;
	string path;

	if (request.getPath() == "/") {
		path = "./html/index.html";

		if (redis.find(path) != redis.end() || insertRedis(path)) {
			response.addHeader("Content-Type", "text/html");
			ok = 1;
		}
	}
	else {
		path = "./" + request.getPath();

		if (redis.find(path) != redis.end() || insertRedis(path)) {
			size_t i = 0;
			string head = request.getHeader("Accept");
			while (i != head.size() && head[i] != ',')
				++i;

			response.addHeader("Content-Type", string(head.begin(), head.begin() + i));
			ok = 1;
		}

	}

	if (ok) {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		if (request.getHeader("Cookie") == "")
			response.addHeader("Set-Cookie", "user_id=yujing;path=/;domain=localhost");

		response.getBody() = redis[path];
	}
	else {
		response.setStatu1(httpresponse::k404NotFound);
		response.setStatu2("Not Found");
		response.setKeepAlive(0);
	}
}