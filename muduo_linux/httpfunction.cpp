#include"httprequest.h"
#include"httpresponse.h"
#include"kmysql.h"
#include"logging.h"

#include<unistd.h>
#include<fcntl.h>
#include<unordered_map>
#include<string>

using namespace::std;

thread_local unordered_map<string, string> redis;

bool login(const string& id, const string& pswd, int type) {
	thread_local kmysql sql;
	bool ok = 0;

	if (sql.isconnected() ||
		sql.connect("localhost", "root", "981227", "test0", 3306, "/var/run/mysqld/mysqld.sock", 0)) {

		string sql_query = "select * from user_map where user_id=\'" + id + "\';";
		if (sql.sendQuery(sql_query)) {
			kmysqlres res(sql.getResult());
			int i = res.getColumn("user_pswd");

			switch (type) {
			case(0):
				if (res.numOfRow() && res[0]->data[i] == pswd)
					ok = 1;

				break;
			case(1):
				if (res.numOfRow() == 0) {
					sql_query = "insert into user_map values(\'" + id + "\',\'" + pswd + "\');";

					if (sql.sendQuery(sql_query))
						ok = 1;
				}

				break;
			default:

				break;
			}
		}
	}

	return ok;
}

bool insertRedis(const string& path) {
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		LOG << "insertRedis失败，errno = " << errno;
		return 0;
	}

	char buf[1024];
	ssize_t nread = 0;
	redis[path] = "";
	string& value = redis[path];

	while ((nread = read(fd, &buf, 1024)) > 0)
		value.append(buf, nread);

	close(fd);
	return 1;
}

void httpCallback(const httprequest& request, const string& content, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	bool ok = 0;
	string path = "./html";

	switch (request.getMethod()) {
	case(httprequest::kGET):
		if (request.getPath() == "/") {
			path += "/index.html";

			if (redis.find(path) != redis.end() || insertRedis(path)) {
				response.addHeader("Content-Type", "text/html");
				ok = 1;
			}
		}
		else {
			path += request.getPath();

			if (redis.find(path) != redis.end() || insertRedis(path)) {
				size_t i = 0;
				string head = request.getHeader("Accept");
				while (i != head.size() && head[i] != ',')
					++i;

				response.addHeader("Content-Type", string(head.begin(), head.begin() + i));
				ok = 1;
			}
		}

		break;
	case(httprequest::kPOST):
		path += "/index.html";

		if (redis.find(path) != redis.end() || insertRedis(path)) {
			size_t left = 9, right = 10;
			while (content[right] != '&')
				++right;
			string id(content.begin() + left, content.begin() + right);

			left = right + 10;
			right = left + 1;
			while (right != content.size() && content[right] != '&')
				++right;
			string pswd(content.begin() + left, content.begin() + right);

			if (request.getPath() == "/login.php")
				ok = login(id, pswd, 0);
			else
				ok = login(id, pswd, 1);

			if (ok) {
				response.addHeader("Content-Type", "text/html");
				response.addHeader("Set-Cookie", "user_id=" + id + ";path=/;domain=localhost");
			}
		}

		break;
	default:

		break;
	}
	
	if (ok) {
		response.setStatu1(httpresponse::k200OK);
		response.setStatu2("OK");
		response.getBody() = redis[path];
	}
	else {
		response.setStatu1(httpresponse::k404NotFound);
		response.setStatu2("Not Found");
		response.setKeepAlive(0);
	}
}