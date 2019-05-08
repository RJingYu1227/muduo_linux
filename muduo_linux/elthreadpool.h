#ifndef ELTHREADPOOL_H
#define ELTHREADPOOL_H

#include"eventloop.h"
#include"tcpserver.h"
#include<pthread.h>

class eventloop;
class tcpserver;

class elthreadpool {
public:
	elthreadpool(int loops = 2);
	~elthreadpool();

	eventloop* getServerLoop() { return serverloop_; }
	eventloop* getIoLoop() { return ioloop_; }
	void start();
	int loopNum() { return loops_; }

private:
	static void* serverThread(void* loop);//Ϊ���̵߳��ã�Ӧ���Ǿ�̬��Ա����
	static void* ioThread(void* loop);

	bool start_;
	int loops_;
	eventloop* serverloop_;
	eventloop* ioloop_;
	pthread_t server_;
	pthread_t io_;
};


#endif // !ELTHREADPOOL_H

