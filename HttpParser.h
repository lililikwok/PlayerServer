#pragma once
#include "Socket.h"
#include "http_parser.h"
#include <map>

// ����http��
class CHttpParser
{
private:
	http_parser m_parser;
	http_parser_settings m_settings; // ���ûص��ṹ��

	// �洢HTTPͷ�ֶκͶ�Ӧ��ֵ
	std::map<Buffer, Buffer> m_HeaderValues;

	// ���ڴ洢HTTP״̬��
	Buffer m_status;

	// ���ڴ洢HTTP�������ӦURL
	Buffer m_url;

	// ���ڴ洢HTTP��Ϣ��
	Buffer m_body;

	// ��ʾHTTP��Ϣ�Ƿ��Ѿ���ȫ���պͽ���
	bool m_complete;

	// ���һ����������HTTPͷ�ֶ���
	Buffer m_lastField;

public:
	CHttpParser();
	~CHttpParser();
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);

public:
	size_t Parser(const Buffer& data);
	// GET POST ... �ο�http_parser.h HTTP_METHOD_MAP��
	unsigned Method() const { return m_parser.method; } // ����
	const std::map<Buffer, Buffer>& Headers() { return m_HeaderValues; }
	const Buffer& Status() const { return m_status; }      // ״̬
	const Buffer& Url() const { return m_url; }            // url
	const Buffer& Body() const { return m_body; }          // ����
	unsigned Errno() const { return m_parser.http_errno; } // ������

protected:
	// �ص����� ������Ӧ��Ա����
	static int OnMessageBegin(http_parser* parser);
	static int OnUrl(http_parser* parser, const char* at, size_t length);
	static int OnStatus(http_parser* parser, const char* at, size_t length);
	static int OnHeaderField(http_parser* parser, const char* at, size_t length);
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
	static int OnHeadersComplete(http_parser* parser);
	static int OnBody(http_parser* parser, const char* at, size_t length);
	static int OnMessageComplete(http_parser* parser);

	// ��Ա��������http����ֵ
	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();
};

// ����url��
class UrlParser
{
public:
	UrlParser(const Buffer& url);
	~UrlParser() {}
	int Parser();
	Buffer operator[](const Buffer& name) const;

	Buffer Protocol() const { return m_protocol; } // Э��
	Buffer Host() const { return m_host; }         // ����
	// Ĭ�Ϸ���80
	int Port() const { return m_port; } // �˿�
	void SetUrl(const Buffer& url);     // uri
	const Buffer Uri() const { return m_uri; };

private:
	Buffer m_url;                      // url
	Buffer m_protocol;                 // Э��
	Buffer m_host;                     // ����
	Buffer m_uri;                      // uri
	int m_port;                        // �˿�
	std::map<Buffer, Buffer> m_values; // ���uri key��value
};