#pragma once
#include "CServer.h"
#include <map>
#include "Logger.h"
#include "HttpParser.h"
#include "MysqlClient.h"
#include "Crypto.h"
#include "json.h"
using json = nlohmann::json;
// ������
#define ERR_RETURN(ret, err)                                             \
    if (ret != 0)                                                        \
    {                                                                    \
        TRACEE("ret=%d errno=%d msg=[%s]", ret, errno, strerror(errno)); \
        return err;                                                      \
    }

#define WARN_CONTINUE(ret)                                               \
    if (ret != 0)                                                        \
    {                                                                    \
        TRACEW("ret=%d errno=%d msg=[%s]", ret, errno, strerror(errno)); \
        continue;                                                        \
    }

// ��������
template <typename _FUNCTION_, typename... _ARGS_>
class CConnectedFunction : public CFunctionBase // ����������
{
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
		: m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) // �󶨺���
	{
	}
	virtual ~CConnectedFunction() {};
	virtual int operator()(CSocketBase* pClient) override // �������������
	{
		return m_binder(pClient); // ���ذ󶨶���
	}

private:
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; // ����
};

template <typename _FUNCTION_, typename... _ARGS_>
class CReveiFunction : public CFunctionBase // ����������
{
public:
	CReveiFunction(_FUNCTION_ func, _ARGS_... args)
		: m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) // �󶨺���
	{
	}
	virtual ~CReveiFunction() {};
	virtual int operator()(CSocketBase* pClient, const Buffer& data) override // �������������
	{
		return m_binder(pClient, data); // ���ذ󶨶���
	}

private:
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; // ����
};

// ҵ���������
class CBusiness
{
public:
	CBusiness()
		: m_connectdcallback(NULL), // ��ʼ���ص�
		m_recvcallback(NULL) {
	};

	virtual int BusinessProcess(CProcess* proc) = 0; // ҵ�����ӽ���

	template <typename _FUNCTION_, typename... _ARGS_>
	int setConnectCallback(_FUNCTION_ func, _ARGS_... args) // ע�����ӻص�����
	{
		m_connectdcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connectdcallback == NULL)
			return -1;
		return 0;
	}

	template <typename _FUNCTION_, typename... _ARGS_>
	int setRecvCallback(_FUNCTION_ func, _ARGS_... args) // ע��ͨ�Żص�����
	{
		m_recvcallback = new CReveiFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_recvcallback == NULL)
			return -1;
		return 0;
	}

protected:
	CFunctionBase* m_connectdcallback; // ���ӻص�
	CFunctionBase* m_recvcallback;     // ͨ�Żص�
};

// ҵ������
class CEdoyunPlayerServer : public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count);

	~CEdoyunPlayerServer();

	int BusinessProcess(CProcess* proc) override; // ҵ�����ӽ���

private:
	int Connected(CSocketBase* pCLient); // ���ӻص�

	int Received(CSocketBase* pCLient, const Buffer& data); // ͨ�Żص�

	int HttpParser(const Buffer& data);

	Buffer MakeResponse(int ret);

private:
	int ThreadFunc(); // �̺߳���

private:
	CEpoll m_epoll;
	ThreadPool m_pool;
	std::map<int, CSocketBase*> m_mapClients; // �ͻ��˻�����
	unsigned m_count;
	CDatabaseClient* m_db;
};

// ��
DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")              // QQ��
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(11)", "18888888888", "") // �ֻ�
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")                      // ����
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")                      // �ǳ�
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "CURRENT_TIMESTAMP", "")
DECLARE_TABLE_CLASS_EDN()

// ntype, name, attr, type, size, default_, check