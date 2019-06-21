#pragma once

#include"buffer.h"
#include<string>
#include<map>

using std::string;

class buffer;

class httprequest {
public:
	enum prasestate {
		kExpectRequestLine,
		kExpectHeaders,
		kExpectBody,
		kPraseDone
	};

	enum method {
		kINVALID,
		kHEAD, kGET, kPOST, kPUT, kDELETE
	};

	enum version {
		kUNKNOWN,
		kHTTP10, kHTTP11
	};

	httprequest() 
		:state_(kExpectRequestLine), 
		method_(kINVALID), 
		version_(kUNKNOWN)
	{}
	
	bool praseRequest(buffer* buffer1);
	bool praseDone()const 
	{ return state_ == kPraseDone; }

	void setVersion(version v) 
	{ version_ = v; }
	version getVersion()const 
	{ return version_; }

	bool setMethod(const char* start, const char* end);
	method getMethod()const 
	{ return method_; }

	void setPath(const char* start, const char* end) 
	{ path_.assign(start, end); }
	const string& getPath()const 
	{ return path_; }

	void setQuery(const char* start, const char* end) 
	{ query_.assign(start, end); }
	const string& getQuery()const 
	{ return query_; }

	void addHeader(const char* start, const char* colon, const char* end);
	string getHeader(const string& key)const;
	const std::map<string, string>& getHeaders()const 
	{ return headers_; }

	void reset();
	void swap(httprequest& that);
	
private:

	bool processRequestLine(const char* start, const char* end);

	prasestate state_;
	method method_;
	version version_;
	string path_;
	string query_;
	std::map<string, string> headers_;

};