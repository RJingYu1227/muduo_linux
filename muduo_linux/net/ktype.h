#pragma once

#include<memory>
#include<functional>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::function;
using std::shared_ptr;

class tcpconnection;

typedef function<void()> functor;
typedef shared_ptr<tcpconnection> tcpconn_ptr;
typedef function<void(const tcpconn_ptr&)> callback;