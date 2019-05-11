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
	void removeConn(tcpconnection* conn);

	static void createQueue(eventloop* loop);
	static eventloop* get_eventloop();

private:
	typedef std::vector<channel*> channel_list;
	//typedef std::vector<tcpconn_ptr> tcpconn_queue;

	//��Ϣ����
	void doFunctors();
	pthread_mutex_t lock_;
	eventfd_t count_;
	std::vector<functor> pending_functors_;
	int wakeupfd_;

	memorypool* m_pool_;

	int epoll_timeout_;
	bool quit_;
	channel_list active_channels_;
	epoller* epoller_;
	bool looping_;
	pthread_t thread_id_;	

};

#endif // !MUDUO_NET_CHANNEL_H

