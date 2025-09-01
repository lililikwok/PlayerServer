#include "CServer.h"

CServer::CServer() : m_pool(4) //�̳߳س�ʼ���Զ������̣߳������Զ��ر�
{
	m_server = NULL;
	m_business = NULL;
}
CServer::~CServer()
{
}
int CServer::Init(CBusiness* business, const Buffer& ip, short port)
{
	// �����ͻ���ҵ���ӽ���
	int ret = 0;
	if (business == NULL)
		return -1;
	m_business = business; // ����ҵ�������


	//std::bind �Գ�Ա���������⴦��
	//	std::bind ��һ���ǳ����ܵĹ��ߡ�����������һ������(&CBusiness::BusinessProcess) ��һ����Ա����ָ��ʱ��
	// ����֪������������ܶ������ڣ�����󶨵�һ������ʵ���ϡ�
	//	���ǣ�std::bind ���Զ����ڶ�������(m_business) ��Ϊ���øó�Ա�����Ķ���ʵ����
	//	���գ�std::bind ������һ��ȫ�µġ��ɵ��õĶ������ǳ�֮Ϊ bound_function����������洢�� m_binder �С�
	// ��� bound_function ���ڲ��߼��ǹ̻��õģ�
	//	����������ʱ�������� m_business ���������ִ�� BusinessProcess ������������& m_process ��Ϊ��������
	//	����ִ�� m_binder() �ȼ���ִ�� m_business->BusinessProcess(&m_process)��
	ret = m_process.SetEntryFunction(&CBusiness::BusinessProcess, m_business, &m_process); // ���÷�������ں���
	if (ret != 0)
		return -2;
	ret = m_process.CreateSubProcess(); // �����ͻ��˴����ӽ���
	if (ret != 0)
		return -3;

	// �����̳߳� �����ͻ�����������
	//m_pool.start(4);

	ret = m_epoll.Create(4);
	if (ret != 0)
		return -5;

	m_server = new CSocket(); // ����������
	if (m_server == NULL)
		return -6;
	ret = m_server->Init(CSockParam(ip, port, SOCK_ISSERVER | SOCK_ISIP | SOCK_ISREUSE)); // ��������ʼ��
	if (ret != 0)
		return -7;
	ret = m_epoll.Add(*m_server, EPollData((void*)m_server)); // ���������Ӽ����ҵ�epoll��
	if (ret != 0)
		return -8;

	for (size_t i = 0; i < m_pool.Size(); i++) // �ύ�ĸ��̵߳ȴ��ͻ�������
	{
		std::future<int> res1 = m_pool.enqueue([this]()
			{ return ThreadFunc(); });
	}
	return 0;
}

int CServer::Run() // ���з�����
{
	while (m_server != NULL)
	{
		usleep(10);
	}
	return 0;
}

int CServer::Close() // �رշ�����
{
	if (m_server)
	{
		CSocketBase* sock = m_server;
		m_server = NULL;
		m_epoll.Del(*sock);
		delete sock;
	}
	m_epoll.Close();
	m_process.SendFD(-1);
	return 0;
}

int CServer::ThreadFunc() // �����������߳� �ȴ��ͻ�������
{
	// TRACEI("epoll %d server %p", (int)m_epoll, m_server);
	//  printf("ip %s �˿�%d ���ڼ����ͻ�������\n", (char *)m_server->m_param.ip, (short)m_server->m_param.port);
	int ret = 0;
	EPEvents events(EVENT_SIZE);
	while ((m_epoll != -1) && (m_server != NULL))
	{
		// printf("[%s]<%s>:%d ���������ڼ����ͻ�������\n", __FILE__, __FUNCTION__, __LINE__);
		ssize_t size = m_epoll.WaitEvents(events, 500);
		// printf("���������ڼ����ͻ������� ��ʱ ret=%d\n", ret);
		if (size < 0)
		{
			printf("���������Ӽ���ʧ��\n");
			break;
		}

		if (size > 0)
		{
			// TRACEI("size=%d event %08X", size, events[0].events);
			for (size_t i = 0; i < size; i++)
			{
				if (events[i].events & EPOLLERR)
				{
					break;
				}
				else if (events[i].events & EPOLLIN)
				{
					if (m_server)
					{
						CSocketBase* pClient = NULL;
						ret = m_server->Link(&pClient); // ����
						// printf("[%s]<%s>:%d %s\n", __FILE__, __FUNCTION__, __LINE__, "�¿ͻ������ӳɹ�");
						if (ret != 0)
							continue;
						ret = m_process.SendSocket(*pClient, *pClient); // ���������ӿͻ����׽��ָ��ӽ���
						int s = *pClient;
						delete pClient;
						if (ret != 0)
						{
							TRACEE("send client %d failed", s);
							continue;
						}
					}
				}
			}
		}
	}
	TRACEI("��������ֹ");
	return 0;
}