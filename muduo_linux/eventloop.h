#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include"channel.h"
#include"epoller.h"
#include"tcpconnection.h"
#include"memorypool.h"
#include<netinet/in.h>
#include<pthread.h>
#include<vector>
#include<memory>
#include<sys/eventfd.h>
#include<functional>

class channel;
class epoller;
class tcpconnection;
class memorypool;

//typedef std::shared_ptr<tcpconnection> tcpconn_ptr;
typedef std::function<void()> functor;

class eventloop {
public:
	eventloop();
	~eventloop();

	void updateThread();
	void loop();
	void quit();
	void assertInLoopThread();
	void updateChannel(channel* ch);
	void removeChannel(channel* ch);
	bool isInLoopThread()const; 
	void runInLoop(const functor& cb);
	void newConn(tcpconnection* &conn, int fd, sockaddr_in* cliaddr);
	void destoryConn(tcpconnection* conn);

	static void createQueue(eventloop* loop,bool n);//n为ture时，创建m_pool_
	static eventloop* get_eventloop();

private:
	typedef std::vector<channel*> channel_list;
	//typedef std::vector<tcpconn_ptr> tcpconn_queue;

	//消息队列
	void handleRead();
	void doFunctors();
	pthread_mutex_t lock_;
	eventfd_t count_;
	std::vector<functor> pending_functors_;
	int eventfd_ = 0;

	memorypool* m_pool_ = nullptr;

	int epoll_timeout_;
	bool quit_ = 0;
	channel_list active_channels_;
	epoller* epoller_;
	bool looping_ = 0;
	pthread_t thread_id_;	

};

#endif // !MUDUO_NET_CHANNEL_H

