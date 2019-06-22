#pragma once

class uncopyable {
private:
	uncopyable(const uncopyable& rhs) = delete;
	uncopyable& operator=(const uncopyable& rhs) = delete;
protected:
	uncopyable() {};
	~uncopyable() {};
};