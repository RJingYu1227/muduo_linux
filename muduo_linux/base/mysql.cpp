#include"mysql.h"

#include"log/logging.h"

using namespace::pax;

bool mysql::connect(const char *host,
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

void mysql::close() {
	if (connected_) {
		mysql_close(&sql_);
		connected_ = 0;
	}
}

bool mysql::sendQuery(const char* query, size_t length) {
	if (connected_) {
		if (mysql_real_query(&sql_, query, length) == 0)
			return 1;

		LOG << mysql_error(&sql_);
	}

	return 0;
}

bool mysql::sendQuery(const std::string& query) {
	if (connected_) {
		if (mysql_real_query(&sql_, query.c_str(), query.length()) == 0)
			return 1;

		LOG << mysql_error(&sql_);
	}

	return 0;
}

MYSQL_RES* mysql::getResult() {
	if (connected_)
		return mysql_store_result(&sql_);
	else
		return nullptr;
}

mysqlres::mysqlres(MYSQL_RES* res) :
	res_(res) {

	rows_.reserve(res_->data->rows);
	MYSQL_ROWS* node = res_->data_cursor;
	for (size_t i = 0; i < res_->data->rows; ++i) {
		rows_[i] = node;
		node = node->next;
	}
}

int mysqlres::getColumn(const std::string& str)const {
	for (unsigned int i = 0; i < res_->data->fields; ++i) {
		if (res_->fields[i].name == str)
			return i;//注意这里
	}

	return -1;
}

int mysqlres::getColumn(const char* str)const {
	for (unsigned int i = 0; i < res_->data->fields; ++i) {
		if (strcmp(res_->fields[i].name, str) == 0)
			return i;//注意这里
	}

	return -1;
}