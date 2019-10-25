#include"logging.h"
#include"httpserver.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"
#include"kmysql.h"

#include<unistd.h>
#include<fcntl.h>
#include<signal.h>

bool login(const string& query) {
	thread_local kmysql sql;
	bool login_ok = 0;

	if (sql.isconnected() || 
		sql.connect("localhost", "root", "981227", "test0", 3306, "/var/run/mysqld/mysqld.sock", 0)) {

		size_t left = 11, right = 12;
		while (query[right] != '&')
			++right;
		string id(query.begin() + left, query.begin() + right);

		left = right + 6;
		right = left + 1;
		while (right != query.size() && query[right] != '&')
			++right;
		string pswd(query.begin() + left, query.begin() + right);

		string sql_query = "select * from user_map where user_id=\'" + id + "\';";
		if (sql.sendQuery(sql_query)) {
			kmysqlres res(sql.getResult());
			int i = res.getColumn("user_pswd");

			switch (query[6]) {
			case('0')://登陆
				if (res.numOfRow() && res[0]->data[i] == pswd)
					login_ok = 1;

				break;
			case('1')://注册
				if (res.numOfRow() == 0) {
					sql_query = "insert into user_map values(\'" + id + "\',\'" + pswd + "\');";

					if (sql.sendQuery(sql_query))
						login_ok = 1;
				}

				break;
			case('2')://修改密码
				if (res.numOfRow() && res[0]->data[i] == pswd) {
					sql_query = "update user_map set user_pswd=\'";
					sql_query.append(query.begin() + right + 7, query.end());
					sql_query = sql_query + "\' where user_id=\'" + id + "\';";

					if (sql.sendQuery(sql_query))
						login_ok = 1;
				}

				break;
			case('3')://注销
				if (res.numOfRow() && res[0]->data[i] == pswd) {
					sql_query = "delete from user_map where user_id=\'" + id + "\';";

					if (sql.sendQuery(sql_query))
						login_ok = 1;
				}

				break;
			default:

				break;
			}
		}
	}

	return login_ok;
}


void httpCallback(const httprequest& request, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	if (request.getPath() == "/login") {
		if (login(request.getQuery())) {
			response.setStatu1(httpresponse::k200OK);
			response.setStatu2("OK");
			response.addHeader("Content-Type", "text/html");
			string now = ktimer::timeToString(ktimer::getUnixTime());
			response.getBody() = "<html><head><title>This is title</title></head>"
				"<body><h1>Hello</h1>Now is " + now +
				"</body></html>";
		}
		else {
			response.setStatu1(httpresponse::k400BadRequest);
			response.setStatu2("Bad Request");
		}
	}
	else {
		response.setStatu1(httpresponse::k404NotFound);
		response.setStatu2("Not Found");
		response.setKeepAlive(0);
	}
}

httpserver* pserver = nullptr;

void sigintHandler(int) {
	if (pserver)
		pserver->stop();
}

int main(int argc, char* argv[]) {
	if (argc != 4)
		return 0;

	signal(SIGINT, sigintHandler);
	signal(SIGPIPE, SIG_IGN);

	logger::createAsyncLogger();

	httpserver server(argv[1], atoi(argv[2]), atoi(argv[3]));
	pserver = &server;
	server.setHttpCallback(httpCallback);
	server.start();

	return 0;
}