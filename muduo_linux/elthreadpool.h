#ifndef ELTHREADPOOL_H
#define ELTHREADPOOL_H

#include"eventloop.h"
#include"tcpserver.h"
#include<pthread.h>
#include<vector>

class eventloop;
class tcpserver;

class elthreadpool {
public:
	elthreadpool(int loops = 2);
	~elthreadpool();

	eventloop* getServerLoop() { return serverloop_; }
	eventloop* getIoLoop();
	void start();
	int loopNum() { return loop_num_; }

private:
	//static void* serverThread(void* loop);//Ϊ���̵߳��ã�Ӧ���Ǿ�̬��Ա����
	static void* ioThread(void* loop);

	bool start_;
	int loop_num_;
	int loop_index_;
	eventloop* serverloop_;
	std::vector<eventloop*> ioloops_;
};


#endif // !ELTHREADPOOL_H

