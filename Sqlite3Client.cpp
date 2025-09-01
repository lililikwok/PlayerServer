#include "Sqlite3Client.h"
#include "Logger.h"

// �������ݿ�   KeyValueЯ�����Ӳ���  args["host"]="test.db"
int CSqlite3Client::Connect(const KeyValue& args)
{
	// �ҵ����ݿ�����
	auto it = args.find("host");
	if (it == args.end())
		return -1;

	if (m_db != NULL) // ���ݿ��Ѿ�����
		return -2;
	int ret = sqlite3_open(it->second, &m_db); // �������ݿ�
	if (ret != 0)
	{
		TRACEE("connect failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -3;
	}
	return 0;
}

// ִ��sql��� �޷���ֵ
int CSqlite3Client::Exec(const Buffer& sql)
{
	printf("sql={%s}\n", (char*)sql);
	if (m_db == NULL) // ���ݿ�δ����
		return -1;

	// ִ�����
	int ret = sqlite3_exec(m_db, sql, NULL, this, NULL);
	if (ret != SQLITE_OK)
	{
		printf("sql={%s}\n", (char*)sql);
		printf("Exec failed:%d [%s]\n", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

// ִ��sql��� ��SELECT���
int CSqlite3Client::Exec(const Buffer& sql, Result __OUT__& result, const _Table_ __IN__& table)
{
	char* errmsg = NULL;
	if (m_db == NULL)
		return -1;
	printf("sql={%s}\n", (char*)sql);

	ExecParam param(this, result, table); // ��װ ��Ϊ�ص�����ֻ������һ��������ַ
	int ret = sqlite3_exec(m_db, sql,
		&CSqlite3Client::ExecCallback, (void*)&param, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("sql={%s}\n", sql);
		printf("Exec failed:%d [%s]\n", ret, errmsg);
		if (errmsg)
			sqlite3_free(errmsg);
		return -2;
	}
	if (errmsg)
		sqlite3_free(errmsg);
	return 0;
}

int CSqlite3Client::StartTransaction()
{
	if (m_db == NULL)
		return -1;
	// ��������
	int ret = sqlite3_exec(m_db, "BEGIN TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE("sql={BEGIN TRANSACTION}");
		TRACEE("BEGIN failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

// �ύ����
int CSqlite3Client::CommitTransaction()
{
	if (m_db == NULL)
		return -1;
	int ret = sqlite3_exec(m_db, "COMMIT TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE("sql={COMMIT TRANSACTION}");
		TRACEE("COMMIT failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

// �ع�����
int CSqlite3Client::RollbackTransaction()
{
	if (m_db == NULL)
		return -1;
	int ret = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", 0, 0, NULL);
	if (ret != SQLITE_OK)
	{
		TRACEE("sql={ROLLBACK TRANSACTION}");
		TRACEE("ROLLBACK failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	return 0;
}

// �ر����ݿ�����
int CSqlite3Client::Close()
{
	if (m_db == NULL)
		return -1;
	int ret = sqlite3_close(m_db);
	if (ret != SQLITE_OK)
	{
		TRACEE("Close failed:%d [%s]", ret, sqlite3_errmsg(m_db));
		return -2;
	}
	m_db = NULL;
	return 0;
}

bool CSqlite3Client::IsConnected()
{
	return m_db != NULL;
}

// ��ѯ�ص�����̬������ ÿһ�м�¼����һ��  count:�ֶ���  values:�ֶ�ֵ names:�ֶ���
int CSqlite3Client::ExecCallback(void* arg, int count, char** values, char** names)
{
	ExecParam* param = (ExecParam*)arg;
	// ���ó�Ա������ʹ�ó�Ա�����Բ�ѯ�������chuli
	return param->obj->ExecCallback(param->result, param->table, count, values, names);
}

int CSqlite3Client::ExecCallback(Result& result, const _Table_& table, int count, char** values, char** names)
{
	PTable pTable = table.Copy(); // ������ѯ���
	if (pTable == nullptr)
	{
		printf("table %s error!\n", (const char*)(Buffer)table);
		return -1;
	}
	for (int i = 0; i < count; i++)
	{
		Buffer name = names[i]; // �ֶ���
		// std::map<Buffer, PField>  Fields;
		auto it = pTable->Fields.find(name); // �ֶ����Ƿ��ڲ�ѯ����д���
		if (it == pTable->Fields.end())
		{
			printf("table %s error!\n", (const char*)(Buffer)table);
			return -2;
		}
		if (values[i] != NULL)                  // �ò�ѯ�ֶ� ���м�¼��ֵ
			it->second->LoadFromStr(values[i]); // �����ֶε�ֵ
	}
	result.push_back(pTable);
	return 0;
}

/************************������********************************/
// �������Ŀ�������
_sqlite3_table_::_sqlite3_table_(const _sqlite3_table_& table)
{
	Database = table.Database;
	Name = table.Name;
	for (size_t i = 0; i < table.FieldDefine.size(); i++)
	{
		PField field = PField(new _sqlite3_field_(*(_sqlite3_field_*)table.FieldDefine[i].get()));
		FieldDefine.push_back(field);
		Fields[field->Name] = field;
	}
}

_sqlite3_table_::~_sqlite3_table_()
{
}

// ������
Buffer _sqlite3_table_::Create()
{ // CREATE TABLE IF NOT EXISTS ���ݿ���.���� (�ж���,����);
	Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + "(\r\n";
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)
			sql += ",";
		sql += FieldDefine[i]->Create();
	}
	sql += ");";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

Buffer _sqlite3_table_::Drop()
{
	Buffer sql = "DROP TABLE " + (Buffer)*this + ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

// �����¼
Buffer _sqlite3_table_::Insert(const _Table_& values)
{ // INSERT INTO ��ȫ�� (��1,...,��n)
	// VALUES(ֵ1,...,ֵn);
	Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
	bool isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_INSERT)
		{
			if (!isfirst)
				sql += ",";
			else
				isfirst = false;
			sql += (Buffer)*values.FieldDefine[i]; // �ֶ���
		}
	}
	sql += ") VALUES (";
	isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_INSERT)
		{
			if (!isfirst)
				sql += ",";
			else
				isfirst = false;
			sql += values.FieldDefine[i]->toSqlStr(); // �ֶ�ֵ
		}
	}
	sql += ");";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

// ɾ����¼
Buffer _sqlite3_table_::Delete(const _Table_& values)
{ // DELETE FROM ��ȫ�� WHERE ����
	Buffer sql = "DELETE FROM " + (Buffer)*this + " ";
	Buffer Where = "";
	bool isfirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (FieldDefine[i]->Condition & SQL_CONDITION)
		{
			if (!isfirst)
				Where += " AND ";
			else
				isfirst = false;
			Where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlStr(); // �ֶ���=�ֶ�ֵ
		}
	}
	if (Where.size() > 0)
		sql += " WHERE " + Where;
	sql += ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

// �޸ļ�¼
Buffer _sqlite3_table_::Modify(const _Table_& values)
{
	// UPDATE ��ȫ�� SET ��1=ֵ1 , ... , ��n=ֵn [WHERE ����];
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_MODIFY)
		{
			if (!isfirst)
				sql += ",";
			else
				isfirst = false;
			sql += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr(); // �ֶ���=�ֶ�ֵ
		}
	}
	// �޸�����
	Buffer Where = "";
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_CONDITION)
		{
			if (!isfirst)
				Where += " AND ";
			else
				isfirst = false;
			Where += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr(); // �ֶ���=�ֶ�ֵ
		}
	}
	if (Where.size() > 0)
		sql += " WHERE " + Where;
	sql += " ;";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

// ��ѯ��������ֶ�
Buffer _sqlite3_table_::Query(const Buffer& Condition)
{ // SELECT ����1 ,����2 ,... ,����n FROM ��ȫ��;
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)
			sql += ',';
		sql += '"' + FieldDefine[i]->Name + "\" ";
	}
	sql += " FROM " + (Buffer)*this + " ";
	if (Condition.size() > 0)
	{
		sql =+ " WHERE " + Condition;
	}
	sql += ";";
	TRACEI("sql = %s", (char*)sql);
	return sql;
}

// ��ֵ��� ���ؿ�������ı�����
PTable _sqlite3_table_::Copy() const
{
	return PTable(new _sqlite3_table_(*this));
}

// ������������ֶ�
void _sqlite3_table_::ClearFieldUsed()
{
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		FieldDefine[i]->Condition = 0;
	}
}

// ����ת�� ���� ���ݿ���.����
_sqlite3_table_::operator const Buffer() const
{
	Buffer Head;
	if (Database.size())
		Head = '"' + Database + "\".";
	return Head + '"' + Name + '"';
}

/************************�ֶβ���********************************/
// �޲ι���
_sqlite3_field_::_sqlite3_field_()
	: _Field_()
{
	nType = TYPE_NULL;
	Value.Double = 0.0;
}

// �вι���
_sqlite3_field_::_sqlite3_field_(int ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
{
	nType = ntype;
	switch (ntype)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		Value.String = new Buffer();
		break;
	}

	Name = name;
	Attr = attr;
	Type = type;
	Size = size;
	Default = default_;
	Check = check;
}

// ��������
_sqlite3_field_::_sqlite3_field_(const _sqlite3_field_& field)
{
	nType = field.nType;
	switch (field.nType)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		Value.String = new Buffer();
		*Value.String = *field.Value.String;
		break;
	}

	Name = field.Name;
	Attr = field.Attr;
	Type = field.Type;
	Size = field.Size;
	Default = field.Default;
	Check = field.Check;
}

// ����
_sqlite3_field_::~_sqlite3_field_()
{
	// �ַ����� new �ˣ���Ҫ���⴦��delete
	switch (nType)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		if (Value.String)
		{
			Buffer* p = Value.String;
			Value.String = NULL;
			delete p;
		}
		break;
	}
}

// ���ش����ֶ���� ���� "id INT PRIMARY_KEY NOT NULL AUTOINCREMENT "
Buffer _sqlite3_field_::Create()
{ //"����" ���� ����
	Buffer sql = '"' + Name + "\" " + Type + Size + " ";
	if (Attr & NOT_NULL)
	{
		sql += " NOT NULL ";
	}
	if (Attr & DEFAULT)
	{
		sql += " DEFAULT " + Default + " ";
	}
	if (Attr & UNIQUE)
	{
		sql += " UNIQUE ";
	}
	if (Attr & PRIMARY_KEY)
	{
		sql += " PRIMARY KEY ";
	}
	if (Attr & CHECK)
	{
		sql += " CHECK( " + Check + ") ";
	}
	if (Attr & AUTOINCREMENT)
	{
		sql += " AUTOINCREMENT ";
	}
	return sql;
}

// Ϊ�ֶ�����ֵ
void _sqlite3_field_::LoadFromStr(const Buffer& str)
{
	switch (nType)
	{
	case TYPE_NULL:
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		Value.Integer = atoi(str);
		break;
	case TYPE_REAL:
		Value.Double = atof(str);
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
		*Value.String = str;
		break;
	case TYPE_BLOB:
		*Value.String = Str2Hex(str);
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
}

// ���� �ֶ���=�ֶ�ֵ
Buffer _sqlite3_field_::toEqualExp() const
{
	Buffer sql = (Buffer)*this + " = ";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
	return sql;
}

// �����ֶ�ֵ
Buffer _sqlite3_field_::toSqlStr() const
{
	Buffer sql = "";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		TRACEW("type=%d", nType);
		break;
	}
	return sql;
}

// ����ת������  �����ֶ���
_sqlite3_field_::operator const Buffer() const
{
	return '"' + Name + '"';
}

// ������ת16����
// һ���ֽڶ�����ռ8λ ʮ������һ�����ֱ�ʾ4λ
Buffer _sqlite3_field_::Str2Hex(const Buffer& data) const
{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (auto ch : data)
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	return ss.str();
}