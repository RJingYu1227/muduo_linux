#pragma once

#include"uncopyable.h"

#include<mysql/mysql.h>
#include<mysql/my_global.h>
#include<string>
#include<vector>

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

class kmysqlres :uncopyable {
public:

	kmysqlres(MYSQL_RES* res);

	~kmysqlres() {
		mysql_free_result(res_);
	}

	int getColumn(const std::string& str)const;
	int getColumn(const char* str)const;
	inline unsigned int numOfField()const;
	inline MYSQL_FIELD* getFields();

	inline size_t numOfRow()const;
	inline MYSQL_ROWS* operator[](size_t idx);

private:

	MYSQL_RES* res_;
	std::vector<MYSQL_ROWS*> rows_;

};

unsigned int kmysqlres::numOfField()const {
	return res_->data->fields;
}

MYSQL_FIELD* kmysqlres::getFields() {
	return res_->fields;
}

size_t kmysqlres::numOfRow()const {
	return res_->data->rows;
}

MYSQL_ROWS* kmysqlres::operator[](size_t idx) {
	return rows_[idx];
}

/*
MYSQL_ROW以链表形式存储
MYSQL_FIELD以数组形式存储
*/