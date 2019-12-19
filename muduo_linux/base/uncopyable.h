#pragma once

namespace pax {

class uncopyable {
protected:

	uncopyable() {};
	~uncopyable() {};

private:

	uncopyable(const uncopyable& rhs) = delete;
	uncopyable& operator=(const uncopyable& rhs) = delete;

};

}//namespace pax