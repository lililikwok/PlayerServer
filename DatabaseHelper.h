/*
���ݿ������
�������
�ֶγ�����
*/
#pragma once
#include "Buffer.h"
#include <map>
#include <list>
#include <memory>
#include <vector>
#include "Logger.h"

#define __OUT__ // ��������
#define __IN__  // �������
#define __INOUT // ���봫������

class _Table_;
class _Field_;

using PTable = std::shared_ptr<_Table_>; // ������ָ��

using PField = std::shared_ptr<_Field_>; // ָ���ֶ�(��)������ָ��

using KeyValue = std::map<Buffer, Buffer>; // ������ʼ�����ݿ����Ӳ��� ���磺args["���ݿ���"]="testdb"

using Result = std::list<PTable>;

// ���ݿ������
class CDatabaseClient
{

public:
    CDatabaseClient() {}
    virtual ~CDatabaseClient() {}

public:
    CDatabaseClient(const CDatabaseClient&) = delete;
    CDatabaseClient& operator=(const CDatabaseClient&) = delete;

public:
    // �������ݿ� ͨ��ӳ��������Ӳ���   ���磺args["���ݿ���"] = 'testdb'
    virtual int Connect(const KeyValue& args) = 0;
    // ִ��sql��䣬��SELECT���
    virtual int Exec(const Buffer& sql) = 0;
    // ִ��sql���,��SELECT���             tableΪִ�в�ѯ�����ı�ͨ�� result���ز�ѯ���
    virtual int Exec(const Buffer& sql, Result __OUT__& result, const _Table_ __IN__& table) = 0;
    // ��������
    virtual int StartTransaction() = 0;
    // �ع�����
    virtual int RollbackTransaction() = 0;
    // �ύ����
    virtual int CommitTransaction() = 0;

    // �ر����ݿ�����
    virtual int Close() = 0;
    // ���ݿ��Ƿ�����
    virtual bool IsConnected() = 0;
};

using FieldArray = std::vector<PField>;    // ����ֶ�
using FieldMap = std::map<Buffer, PField>; // �ֶ���=�ֶ�

// ���������  ����sql���
class _Table_
{
public:
    _Table_() {}
    virtual ~_Table_() {}

    // ������
    virtual Buffer Create() = 0;
    // ɾ����
    virtual Buffer Drop() = 0;
    // ��ɾ�Ĳ�
    virtual Buffer Insert(const _Table_& values) = 0;
    virtual Buffer Delete(const _Table_& values) = 0;
    virtual Buffer Modify(const _Table_& values) = 0;
    virtual Buffer Query(const Buffer& Condition = "") = 0;

    // ����һ�����ڱ�Ķ���
    virtual PTable Copy() const = 0;
    virtual void ClearFieldUsed() = 0;

public:
    // ��ȡ���ȫ��(����ת�����������)  const Buffer tbName = _Table_; ���� Buffer tbName = _Table_;
    virtual operator const Buffer() const = 0;

public:
    Buffer Database;        // ���ݿ���
    Buffer Name;            // ����
    FieldArray FieldDefine; // ��������ֶ�
    FieldMap Fields;        // ��������ֶ������ֶε�ӳ��
};

enum Condition
{
    SQL_INSERT = 1,   // ����������ֶ�
    SQL_MODIFY = 2,   // �����޸ĵ��ֶ�
    SQL_CONDITION = 4 // ��������ѯ�������ֶ�
};

// Լ������
enum Attr
{
    NONE = 0,
    NOT_NULL = 1,
    DEFAULT = 2,
    UNIQUE = 4,
    PRIMARY_KEY = 8,
    CHECK = 16,
    AUTOINCREMENT = 32
};

// ���ݿ�����
using SqlType = enum {
    TYPE_NULL = 0,
    TYPE_BOOL = 1,
    TYPE_INT = 2,
    TYPE_DATETIME = 4, // ���ں�ʱ������
    TYPE_REAL = 8,     // ʵ��
    TYPE_VARCHAR = 16, // �ַ���
    TYPE_TEXT = 32,    // �ı�
    TYPE_BLOB = 64     // ���������� ͼ����Ƶ����Ƶ
};

// �ֶλ���
class _Field_
{
public:
    _Field_() {}
    _Field_(const _Field_& field) // ��������
    {
        Name = field.Name;
        Type = field.Type;
        Attr = field.Attr;
        Default = field.Default;
        Check = field.Check;
    }
    virtual _Field_& operator=(const _Field_& field) // ��ֵ����
    {
        if (this != &field)
        {
            Name = field.Name;
            Type = field.Type;
            Attr = field.Attr;
            Default = field.Default;
            Check = field.Check;
        }
        return *this;
    }
    virtual ~_Field_() {}

public:
    // �����ֶ�
    virtual Buffer Create() = 0;
    // �����ֶ�ֵ
    virtual void LoadFromStr(const Buffer& str) = 0;
    // ���� �ֶ���=ֵ
    virtual Buffer toEqualExp() const = 0;
    // ���� ֵ
    virtual Buffer toSqlStr() const = 0;
    // ����ת����������� ȡ�ֶ��� const Buffer FieldName = _Field_;
    virtual operator const Buffer() const = 0;

public:
    Buffer Name;    // �ֶ���
    Buffer Type;    // �ֶ���������
    Buffer Size;    // �ֶδ�С
    unsigned Attr;  // Լ������ �� Attr ö������
    Buffer Default; // Ĭ��ֵ
    Buffer Check;   // ����Լ�� �����ֶ�ȡֵ��Χ

public:
    // �ֶ�������ʲô����
    unsigned Condition; // �� Condition ö������
    // �ֶ�ֵ
    union
    {
        bool Bool;
        int Integer;
        double Double;
        Buffer* String;
    } Value;
    // �ֶ����� �����ݿ�����ö��
    int nType;
    /*
        // ���ݿ��ֶ�����
        using SqlType = enum {
            TYPE_NULL = 0,
            TYPE_BOOL = 1,
            TYPE_INT = 2,
            TYPE_DATETIME = 4,
            TYPE_REAL = 8,
            TYPE_VARCHAR = 16,
            TYPE_TEXT = 32,
            TYPE_BLOB = 64
        };
    */
};