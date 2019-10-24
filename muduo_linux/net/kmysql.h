#pragma once

#include"uncopyable.h"

#include<mysql/mysql.h>
#include<mysql/my_global.h>
#include<string>

class kmysql :uncopyable {
public:

	kmysql() :
		connected_(0) {

	}

	~kmysql() {
		if (connected_)
			mysql_close(&sql_);
	}

	bool isconnected()const { return connected_; }
	bool connect(const char *host,
		const char *user,
		const char *passwd,
		const char *db,
		unsigned int port,
		const char *unix_socket,
		unsigned long clientflag);
	void close();

	bool sendQuery(const char* query, size_t length);
	bool sendQuery(const std::string& query);
	MYSQL_RES* getResult();

private:

	bool connected_;
	MYSQL sql_;

};