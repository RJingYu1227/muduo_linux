#include"httprequest.h"
#include"buffer.h"

#include<assert.h>
#include<algorithm>

bool httprequest::setMethod(const char* start, const char* end) {
	assert(method_ == kINVALID);
	string m(start, end);
	
	if (m == "HEAD")
		method_ = kHEAD;
	else if (m == "GET")
		method_ = kGET;
	else if (m == "POST")
		method_ = kPOST;
	else if (m == "PUT")
		method_ = kPUT;
	else if (m == "DELETE")
		method_ = kDELETE;

	return method_ != kINVALID;
}

void httprequest::addHeader(const char* start, const char* colon, const char* end) {
	string key(start, colon);
	++colon;
	while (colon < end && *colon == ' ')
		++colon;

	while (end > colon && *(end - 1) == ' ')
		--end;
	string value(colon, end);
	for (auto& ch : value) {
		if ('A' <= ch && ch <= 'Z')
			ch = static_cast<char>(ch - 'A' + 'a');
	}

	headers_[key] = value;
}

string httprequest::getHeader(const string& key)const {
	auto iter = headers_.find(key);
	if (iter != headers_.end())
		return iter->second;
	else
		return "";
}

void httprequest::reset() {
	state_ = kExpectRequestLine;
	method_ = kINVALID;
	version_ = kUNKNOWN;
	path_.clear();
	query_.clear();
	headers_.clear();
	length_ = 0;
}

/*
void httprequest::swap(httprequest& that) {
	std::swap(state_, that.state_);
	std::swap(method_, that.method_);
	std::swap(version_, that.version_);
	path_.swap(that.path_);
	query_.swap(that.query_);
	headers_.swap(that.headers_);
}
*/

bool httprequest::processRequestLine(const char* start, const char* end) {
	const char* space = std::find(start, end, ' ');

	if (space != end && setMethod(start, space)) {
		start = space + 1;
		space = std::find(start, end, ' ');
		if (space == end)
			return 0;

		const char* quemk = std::find(start, space, '?');
		path_.assign(start, quemk);
		query_.assign(quemk, space);

		start = space + 1;
		if (!(end - start == 8 && std::equal(start, end - 1, "HTTP/1.")))
			return 0;

		if (*(end - 1) == '0')
			version_ = kHTTP10;
		else if (*(end - 1) == '1')
			version_ = kHTTP11;
		else
			return 0;
	}

	return 1;
}

bool httprequest::parseRequest(buffer* buffer1) {
	while (1) {
		if (state_ == kExpectBody) {
			if (buffer1->usedBytes() >= length_)
				state_ = kParseDone;
			break;
		}

		const char* crlf = buffer1->findCRLF();
		if (crlf == NULL)
			break;
		const char* start = buffer1->beginPtr();
		buffer1->retrieve(crlf + 2 - start);

		if (state_ == kExpectRequestLine) {
			if (processRequestLine(start, crlf))
				state_ = kExpectHeaders;
			else
				return 0;
		}
		else if (state_ == kExpectHeaders) {
			const char* colon = std::find(start, crlf, ':');
			if (colon != crlf)
				addHeader(start, colon, crlf);
			else if (headers_.find("Content-Length") == headers_.end()) {
				state_ = kParseDone;
				break;
			}
			else {
				length_ = atoi(headers_["Content-Length"].c_str());
				if (length_ == 0) {
					state_ = kParseDone;
					break;
				}
				state_ = kExpectBody;
			}
		}
	}

	return 1;
}