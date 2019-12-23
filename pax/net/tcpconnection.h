#pragma once

#include<pax/base/buffer.h>
#include<pax/base/socket.h>

#include<pax/net/channel.h>

#include<memory>

namespace pax {

class eventloop;
class tcpconnection;

typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
typedef std::function<void(const tcpconn_ptr&)> event_callback;

class tcpconnection :uncopyable, public std::enable_shared_from_this<tcpconnection> {
public:

	tcpconnection(eventloop* loop, int fd, sockaddr_in& cliaddr);
	~tcpconnection();

	eventloop* getLoop() { return loop_; }
	void setConnectedCallback(const event_callback& cb) { connectedCallback = cb; }
	void setClosedCallback(const event_callback& cb) { closedCallback = cb; }
	void setRecvDoneCallback(const event_callback& cb) { recvDoneCallback = cb; }
	void setSendDoneCallback(const event_callback& cb) { sendDoneCallback = cb; }
	void setHighWaterCallback(const event_callback& cb, size_t highwater) {
		highWaterCallback = cb;
		watermark_ = highwater;
	}

	//tcp选项，注意线程安全
	void setTcpNodelay(bool on);
	void setKeepalive(bool on);

	void startRead();
	void stopRead();
	void send(buffer* data);
	void send(const std::string& data);

	buffer* getRecvBuffer() { return &buffer1_; }
	buffer* getSendBuffer() { return &buffer2_; }

	void start();
	void forceClose();//极端情况下并非线程安全
	void forceCloseWithDelay(double seconds);//秒为单位
	void shutDown();//优雅关闭

	//任意指针
	void setPtr(void* ptr) { ptr_ = ptr; }
	void* getPtr() { return ptr_; }

	int getFd()const { return fd_; }
	uint16_t getPort()const { return socket_.getPort(); }
	const char* getIp()const { return socket_.getIp(); }
	bool connected()const { return state_ == kConnected; }
	bool disConnected()const { return state_ == kDisConnected; }

private:
	enum state {
		kConnecting,
		kConnected,
		kDisConnecting,
		kDisConnected
	};

	//建议使用shared_from_this()，不然不是线程安全的
	void startReadInLoop();
	void stopReadInLoop();
	void sendInLoop1(const std::string& data);
	void sendInLoop2(const char* data, size_t len);
	void shutDownInLoop();

	void handleRead();
	void handleClose();
	void handleWrite();
	void handleError();

	eventloop* loop_;
	void* ptr_;

	int fd_;
	state state_;
	size_t watermark_;

	socket socket_;
	buffer buffer1_;
	buffer buffer2_;
	channel channel_;

	event_callback connectedCallback;
	event_callback closedCallback;
	event_callback recvDoneCallback;
	event_callback sendDoneCallback;
	event_callback highWaterCallback;

};

}//namespace pax