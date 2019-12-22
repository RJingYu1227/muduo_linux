#pragma once

#include<pax/base/uncopyable.h>

#include<mysql/mysql.h>
#include<mysql/my_global.h>
#include<string>
#include<vector>

namespace pax {

class mysql :uncopyable {
public:

	mysql() :
		connected_(0) {

	}

	~mysql() {
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

class mysqlres :uncopyable {
public:

	mysqlres(MYSQL_RES* res);

	~mysqlres() {
		mysql_free_result(res_);
	}

	int getColumn(const std::string& str)const;
	int getColumn(const char* str)const;
	unsigned int numOfField()const;
	MYSQL_FIELD* getFields();

	size_t numOfRow()const;
	MYSQL_ROWS* operator[](size_t idx);

private:

	MYSQL_RES* res_;
	std::vector<MYSQL_ROWS*> rows_;

};

inline unsigned int mysqlres::numOfField()const {
	return res_->data->fields;
}

inline MYSQL_FIELD* mysqlres::getFields() {
	return res_->fields;
}

inline size_t mysqlres::numOfRow()const {
	return res_->data->rows;
}

inline MYSQL_ROWS* mysqlres::operator[](size_t idx) {
	return rows_[idx];
}
/*
MYSQL_ROW以链表形式存储
MYSQL_FIELD以数组形式存储
*/

}//namespace pax