#pragma once
#include "Buffer.h"
#include "DatabaseHelper.h"
#include <mysql/mysql.h>

// ���ݿ������
class CMysqlClient
	: public CDatabaseClient
{

public:
	CMysqlClient()
	{
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	virtual ~CMysqlClient()
	{
		Close(); // �ر����ݿ�����
	}

public:
	CMysqlClient(const CMysqlClient&) = delete;
	CMysqlClient& operator=(const CMysqlClient&) = delete;

public:
	// �������ݿ�
	virtual int Connect(const KeyValue& args);
	// ִ��sql���
	virtual int Exec(const Buffer& sql);
	// �������ִ��sql���
	virtual int Exec(const Buffer& sql, Result __OUT__& result, const _Table_ __IN__& table);
	// ��������
	virtual int StartTransaction();
	// �ύ����
	virtual int CommitTransaction();
	// �ع�����
	virtual int RollbackTransaction();
	// �ر�����
	virtual int Close();
	// �Ƿ����� true��ʾ������ false��ʾδ����
	virtual bool IsConnected();

private:
	MYSQL* m_db;
	bool m_bInit; // Ĭ����false ��ʾû�г�ʼ�� ��ʼ����Ϊtrue ��ʾ������

private:
	// �ڲ��� ��װ�������������
	class ExecParam
	{
	public:
		ExecParam(CMysqlClient* obj, Result& result, const _Table_& table)
			: obj(obj), result(result), table(table)
		{
		}
		CMysqlClient* obj;
		Result& result;
		const _Table_& table;
	};
};

// �������
class _mysql_table_ : public _Table_
{
public:
	_mysql_table_() : _Table_() {}
	_mysql_table_(const _mysql_table_& table); // ��������
	virtual ~_mysql_table_();

	// ���ش������SQL���
	virtual Buffer Create();
	// ɾ����
	virtual Buffer Drop();

	// ��������ɾ�Ĳ�
	virtual Buffer Insert(const _Table_& values);
	virtual Buffer Delete(const _Table_& values);
	virtual Buffer Modify(const _Table_& values);
	virtual Buffer Query(const Buffer& Condition = "");
	// ����һ�����ڱ�Ķ���
	virtual PTable Copy() const;
	virtual void ClearFieldUsed();

public:
	// ��ȡ���ȫ��
	virtual operator const Buffer() const;
};

// �ֶβ�����
class _mysql_field_ : public _Field_
{
public:
	_mysql_field_();

	_mysql_field_( // �вι���
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check);
	_mysql_field_(const _mysql_field_& field); // ��������
	virtual ~_mysql_field_();

	// ���ش����ֶ���� ���� "id INT PRIMARY_KEY NOT NULL AUTOINCREMENT "
	virtual Buffer Create();

	// ���� �ֶ�����nType ����Valueֵ
	virtual void LoadFromStr(const Buffer& str);

	// where�������ʹ��
	virtual Buffer toEqualExp() const; // ���� �ֶ���=ֵ
	virtual Buffer toSqlStr() const;   // ���� ֵ

	// ��������ת�� �����ֶε�ȫ��
	virtual operator const Buffer() const;

private:
	// ������ת16����
	Buffer Str2Hex(const Buffer& data) const;
};

// ������Ԥ����
// nameΪ������  baseΪ������
#define DECLARE_TABLE_CLASS(name, base)                                 \
    class name : public base                                            \
    {                                                                   \
    public:                                                             \
        virtual PTable Copy() const { return PTable(new name(*this)); } \
        name() : base()                                                 \
        {                                                               \
            Name = #name;

#define DECLARE_MYSQL_FIELD(ntype, name, attr, type, size, default_, check)               \
    {                                                                                     \
        PField field(new _mysql_field_(ntype, #name, attr, type, size, default_, check)); \
        FieldDefine.push_back(field);                                                     \
        Fields[#name] = field;                                                            \
    }

#define DECLARE_TABLE_CLASS_EDN() \
    }                             \
    }                             \
    ;