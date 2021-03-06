﻿#pragma once

#include<pax/base/buffer.h>

#include<stdio.h>

#include<map>
#include<string>

using std::string;

class httpresponse {
public:
	enum statu {
		kUNKNOWN,
		k200OK = 200,
		k400BadRequest = 400,
		k401Unauthorized = 401,
		k403Forbidden = 403,
		k404NotFound = 404,
	};

	explicit httpresponse(bool alive)
		:statu1_(kUNKNOWN),
		alive_(alive) {

	};

	void addHeader(const string& key, const string& value)
	{
		headers_[key + ": "] = (value + "\r\n");
	}
	void eraseHeader(const string& key)
	{
		headers_.erase(key + ": ");
	}

	string& getBody() { return body_; }
	void setStatu1(statu value) { statu1_ = value; }
	void setStatu2(const string& value) { statu2_ = value + "\r\n"; }
	void setKeepAlive(bool alive) { alive_ = alive; }
	bool keepAlive()const { return alive_; }

	void appendToBuffer(pax::buffer& buffer2);

private:

	std::map<string, string> headers_;
	string body_;
	statu statu1_;
	string statu2_;
	bool alive_;

};