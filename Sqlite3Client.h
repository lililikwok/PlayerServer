#pragma once
#include "Buffer.h"
#include "DatabaseHelper.h"
#include "sqlite3/sqlite3.h"

// ���ݿ������
class CSqlite3Client
	: public CDatabaseClient
{

public:
	CSqlite3Client()
	{
		m_db = NULL;
		m_stmt = NULL;
	}
	virtual ~CSqlite3Client()
	{
		Close(); // �ر����ݿ�����
	}

public:
	CSqlite3Client(const CSqlite3Client&) = delete;
	CSqlite3Client& operator=(const CSqlite3Client&) = delete;

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
	// argΪ���ûص�ʱ�������Ĳ���
	// SELECT��ѯʱ��ÿ��ѯ��һ�м�¼�������һ��ExecCallback�ص�
	// count��ѯ�����ֶ���Ŀ  names[i]Ϊһ�м�¼�е�i���ֶ�����  values[i]Ϊһ�м�¼�е�i���ֶε�ֵ
	static int ExecCallback(void* arg, int count, char** values, char** names);
	int ExecCallback(Result& result, const _Table_& table, int count, char** values, char** names);

private:
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;

private:
	// �ڲ��� ��װ�������������
	class ExecParam
	{
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table)
			: obj(obj), result(result), table(table)
		{
		}
		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};

// �������
class _sqlite3_table_ : public _Table_
{
public:
	_sqlite3_table_() : _Table_() {}
	_sqlite3_table_(const _sqlite3_table_& table); // ��������
	virtual ~_sqlite3_table_();

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
	//�����һЩ����
	//Buffer Database;        // ���ݿ���
	//Buffer Name;            // ����
	//FieldArray FieldDefine; // ��������ֶ�
	//FieldMap Fields;        // ��������ֶ������ֶε�ӳ��
};

// �ֶβ�����
class _sqlite3_field_ : public _Field_
{
public:
	_sqlite3_field_();
	_sqlite3_field_( // �вι���
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check);
	_sqlite3_field_(const _sqlite3_field_& field); // ��������
	virtual ~_sqlite3_field_();

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

#define DECLARE_FIELD(ntype, name, attr, type, size, default_, check)                       \
    {                                                                                       \
        PField field(new _sqlite3_field_(ntype, #name, attr, type, size, default_, check)); \
        FieldDefine.push_back(field);                                                       \
        Fields[#name] = field;                                                              \
    }

#define DECLARE_TABLE_CLASS_EDN() \
    }                             \
    }                             \
    ;