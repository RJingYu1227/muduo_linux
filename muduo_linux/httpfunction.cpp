#include"httprequest.h"
#include"httpresponse.h"
#include"kmysql.h"
#include"logging.h"

#include<unistd.h>
#include<fcntl.h>
#include<unordered_map>
#include<string>
#include<initializer_list>

using namespace::std;

thread_local unordered_map<string, string> redis;

size_t parseContent(const string& content, initializer_list<string*> values) {
	size_t count = 0;
	size_t idx = 0;
	size_t q_size = content.size();

	for (auto str : values) {
		while (idx != q_size && content[idx] != '=')
			++idx;

		if (idx == q_size)
			return count;

		size_t temp = idx + 1;
		while (idx != q_size && content[idx] != '&')
			++idx;
		str->assign(content.begin() + temp, content.begin() + idx);
		++count;
	}

	return count;
}

bool checkLoginValues(initializer_list<string*> values) {
	string* pstr = nullptr;

	for (size_t i = 0; i < values.size(); ++i) {
		pstr = values.begin()[i];

		if (i < 2) {//name, pswd
			if (pstr->size() < 6 || pstr->size() > 16)
				return 0;

			for (auto ch : *pstr)
				if (isalnum(ch) == false)
					return 0;
		}
		else if (i == 2) {//repswd
			if (*values.begin()[1] != *pstr)
				return 0;
		}
		else if (i == 3) {//tel
			if (pstr->size() != 11)
				return 0;

			for (auto ch : *pstr)
				if (isdigit(ch) == false)
					return 0;
		}
		else
			break;
	}

	return 1;
}

bool login(initializer_list<string*> values) {
	thread_local kmysql sql;
	bool ok = 0;

	if (checkLoginValues(values) == false)
		return 0;

	if (sql.isconnected() ||
		sql.connect("localhost", "root", "hanchunzi1998", "test", 3306, "/var/run/mysqld/mysqld.sock", 0)) {

		auto id = values.begin()[0];
		auto pswd = values.begin()[1];

		string sql_query = "select * from user where user_id=\'" + *id + "\';";
		if (sql.sendQuery(sql_query)) {
			kmysqlres res(sql.getResult());
			int i = res.getColumn("user_pswd");

			switch (values.size()) {
			case(2):
				if (res.numOfRow() && res[0]->data[i] == *pswd)
					ok = 1;

				break;
			case(4):
				if (res.numOfRow() == 0) {
					auto phone = values.begin()[3];
					sql_query = "insert into user values(\'" + *id + "\',\'" + *pswd + "\',\'" + *phone + "\');";

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
		LOG << "insertRedis失败，errno = " << errno << " path = " << path;
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

		//content格式为username=xxx&password=xxx&repassword=xxx&tel=xxx&submit=xxx
		if (redis.find(path) != redis.end() || insertRedis(path)) {
			string id, pswd;
			if (request.getPath() == "/login.php") {
				if (parseContent(content, { &id,&pswd }) == 2)
					ok = login({ &id,&pswd });
			}
			else if (request.getPath() == "/regist.php") {
				string repswd, phone;
				if (parseContent(content, { &id,&pswd,&repswd,&phone }) == 4)
					ok = login({ &id,&pswd,&repswd,&phone });
			}

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