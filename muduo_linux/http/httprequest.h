#pragma once

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
		kParseDone
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
		version_(kUNKNOWN),
		length_(0)
	{}
	
	bool parseRequest(buffer* buffer1);
	bool parseDone()const 
	{ return state_ == kParseDone; }

	version getVersion()const 
	{ return version_; }

	method getMethod()const 
	{ return method_; }

	const string& getPath()const 
	{ return path_; }

	const string& getQuery()const 
	{ return query_; }

	string getHeader(const string& key)const;
	const std::map<string, string>& getHeaders()const 
	{ return headers_; }

	size_t getLength()const 
	{ return length_; }

	void reset();
	//void swap(httprequest& that);
	
private:

	bool processRequestLine(const char* start, const char* end);
	bool setMethod(const char* start, const char* end);
	void addHeader(const char* start, const char* colon, const char* end);

	prasestate state_;
	method method_;
	version version_;
	string path_;
	string query_;
	std::map<string, string> headers_;
	size_t length_;

};