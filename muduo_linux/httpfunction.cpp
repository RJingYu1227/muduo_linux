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

/*
简易的线程私有的内存数据库
用于保存一般性资源文件，以path为key，文件内容为value
*/
thread_local unordered_map<string, string> redis;

/*
解析query语句
返回值为解析出的value数量，小于等于传入的values数量
*/
size_t parseContent(const string& content, initializer_list<string*> values) {
	size_t count = 0;//计数器，计数解析出的value数量
	size_t idx = 0;//content的游标
	size_t q_size = content.size();//content的大小，即字符个数

	for (auto str : values) {
		while (idx != q_size && content[idx] != '=')
			++idx;

		if (idx == q_size)
			return count;

		size_t temp = idx + 1;
		while (idx != q_size && content[idx] != '&')
			++idx;

		//将解析出来的value复制到目标缓冲区
		str->assign(content.begin() + temp, content.begin() + idx);
		++count;
	}

	return count;
}

/*
验证用户名，密码，电话号码是否合法
用户名密码6-16位，字母数字
电话号码11位，数字
传入的参数应按username，password，repassword，tel的顺序传入
*/
bool checkLoginValues(initializer_list<string*> values) {
	string* pstr = nullptr;

	for (size_t i = 0; i < values.size(); ++i) {
		pstr = values.begin()[i];

		if (i < 2) {//name, pswd
			if (pstr->size() < 6 || pstr->size() > 16)
				return 0;

			for (auto ch : *pstr)
				if (isalnum(ch) == false)//标准库函数，判断是否是字母或数字
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
				if (isdigit(ch) == false)//标准库函数，判断是否是数字
					return 0;
		}
		else
			break;
	}

	return 1;
}

/*
处理登陆注册的函数
传入参数为username，password或者username，password，repassword，tel
*/
bool login(initializer_list<string*> values) {
	thread_local kmysql sql;
	//线程私有的数据库连接，保持一个数据库连接，以避免重复的建立关闭socket连接带来的开销
	bool ok = 0;

	if (checkLoginValues(values) == false)
		return 0;
	//如果不合法，直接返回0，登陆或者注册失败

	if (sql.isconnected() ||
		sql.connect("localhost", "root", "hanchunzi1998", "test", 3306, "/var/run/mysqld/mysqld.sock", 0)) {
		//数据库连接成功则执行下面的语句

		auto id = values.begin()[0];
		auto pswd = values.begin()[1];

		string sql_query = "select * from user where user_id=\'" + *id + "\';";
		//从数据库查询用户名（主键）所在的元组

		if (sql.sendQuery(sql_query)) {
			//查询成功则执行下面的语句
			kmysqlres res(sql.getResult());//将查询结果保存到本地缓冲区
			int i = res.getColumn("user_pswd");//寻找user_pswd所在的列

			switch (values.size()) {
			case(2):
				//查询结果不为空且user_pswd列的值与pswd匹配则登陆成功
				if (res.numOfRow() && res[0]->data[i] == *pswd)
					ok = 1;

				break;
			case(4):
				//查询结果为空，且insert语句成功则注册成功
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

/*
将指定文件读取进redis的函数
path为路径名，返回是否读取成功
*/
bool insertRedis(const string& path) {
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		LOG << "insertRedis失败，errno = " << errno << " path = " << path;
		return 0;
	}

	char buf[1024];//因为使用了协程，所以缓冲区大小不应过大，避免栈溢出，这一段可以优化
	ssize_t nread = 0;
	redis[path] = "";
	string& value = redis[path];

	//循环读取，直到返回-1，此时errno = EAGIN
	while ((nread = read(fd, &buf, 1024)) > 0)
		value.append(buf, nread);

	close(fd);
	return 1;
}

/*
完整解析一次http请求后，会调用此函数进行httpresponse的填充
可以处理get请求，仅用于请求一般性资源文件
可以处理post请求，仅用于登陆注册
只返回200或404状态码
*/
void httpCallback(const httprequest& request, const string& content, httpresponse& response) {
	response.addHeader("Server", "RJingYu");
	bool ok = 0;
	string path = "./html";

	switch (request.getMethod()) {
	case(httprequest::kGET):
		if (request.getPath() == "/") {
			path += "/index.html";

			//返回主页面
			if (redis.find(path) != redis.end() || insertRedis(path)) {
				response.addHeader("Content-Type", "text/html");
				ok = 1;
			}
		}
		else {
			path += request.getPath();

			//返回其它资源文件
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

			//login成功则设置cookie，返回主页面
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