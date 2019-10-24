#include"kmysql.h"
#include"logging.h"

bool kmysql::connect(const char *host,
	const char *user,
	const char *passwd,
	const char *db,
	unsigned int port,
	const char *unix_socket,
	unsigned long clientflag) {

	if (connected_)
		return 1;

	mysql_init(&sql_);
	if (mysql_real_connect(&sql_, host, user, passwd, db, port, unix_socket, clientflag) != nullptr)
		connected_ = 1;
	else {
		connected_ = 0;
		LOG << mysql_error(&sql_);
	}

	return connected_;
}

void kmysql::close() {
	if (connected_) {
		mysql_close(&sql_);
		connected_ = 0;
	}
}

bool kmysql::sendQuery(const char* query, size_t length) {
	if (connected_) {
		if (mysql_real_query(&sql_, query, length) == 0)
			return 1;

		LOG << mysql_error(&sql_);
	}

	return 0;
}

bool kmysql::sendQuery(const std::string& query) {
	if (connected_) {
		if (mysql_real_query(&sql_, query.c_str(), query.length()) == 0)
			return 1;

		LOG << mysql_error(&sql_);
	}

	return 0;
}

MYSQL_RES* kmysql::getResult() {
	if (connected_)
		return mysql_store_result(&sql_);
	else
		return nullptr;
}