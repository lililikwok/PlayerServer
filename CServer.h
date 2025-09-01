#pragma once
#include "Socket.h"
#include "Epoll.h"
#include "process.h"
//#include "threadpool.h"
#include "MyThreadPool.h"
#include "EdoyunPlayerServer.h"
#include "Logger.h"
#include <future>
#define PORT 9527
#define IP "0.0.0.0"
class CBusiness;
// ��������
class CServer
{

public:
	CServer();
	~CServer();
	CServer(const CServer&) = delete;
	CServer& operator=(const CServer&) = delete;

	int Init(CBusiness* business, const Buffer& ip = IP, short port = PORT);
	int Run();
	int Close();

private:
	int ThreadFunc(); // �������̺߳���

private:
	ThreadPool m_pool;     // �̳߳�
	CSocketBase* m_server; // ������
	CEpoll m_epoll;
	CProcess m_process;    // ����������
	CBusiness* m_business; // ҵ��ģ��       
};