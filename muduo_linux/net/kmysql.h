#pragma once

#include"uncopyable.h"

#include<mysql/mysql.h>
#include<mysql/my_global.h>
#include<string>

class kmysql :uncopyable {
public:
	
	~kmysql() {
		mysql_free_result(res_);
		mysql_close(&sql_);
	}

	void setQuery(const char* query) 
	{ query_ = query; }

	void send() { 
		mysql_free_result(res_);
		mysql_query(&sql_, query_.c_str()); 
	}

private:

	MYSQL sql_;

	std::string query_;

	MYSQL_RES* res_;
	MYSQL_FIELD* field_;
	MYSQL_ROW row_;

};

//未完待续