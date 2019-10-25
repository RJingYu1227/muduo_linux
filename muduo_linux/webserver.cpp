#include"logging.h"
#include"httpserver.h"
#include"httprequest.h"
#include"httpresponse.h"
#include"ktimer.h"
#include"kmysql.h"

#include<unistd.h>
#include<fcntl.h>
#include<signal.h>

bool login(char type, const string& id, const string& pswd) {
	thread_local kmysql sql;
	string query = "select * from user_map where user_id=\'" + id + "\';";

	bool login_ok = 0;
	if (sql.isconnected() || sql.connect("localhost", "root", "981227", "test0", 3306, "/var/run/mysqld/mysqld.sock", 0)) {
		if (sql.sendQuery(query)) {
			kmysqlres res(sql.getResult());
			int i = res.getColumn("user_pswd");

			if (type == '0' && res.numOfRow()) {
				if (res[0]->data[i] == pswd)
					login_ok = 1;
			}
			else if (type == '1' && !res.numOfRow()) {
				query = "insert into user_map values(\'" + id + "\',\'" + pswd + "\');";
				if (sql.sendQuery(query))
					login_ok = 1;
			}
		}
	}

	return login_ok;
}


void httpCallback(const httprequest& request, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	if (request.getPath() == "/login") {
		int left = 11;
		string id, pswd;
		while (request.getQuery()[left] != '&') {
			id += request.getQuery()[left];
			++left;
		}
		left += 6;
		pswd.assign(request.getQuery().begin() + left, request.getQuery().end());

		bool login_ok = login(request.getQuery()[6], id, pswd);
		if (login_ok) {
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