#include "mysqlParameter.h"
#include "backend/databaseLayer/databaseLayer.h"

// ctor
ibDatabaseParameterMySQL::ibDatabaseParameterMySQL()
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_NULL;
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(const wxString& strValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_STRING;
	m_strValue = strValue;
	m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
	if (wxEmptyString == strValue)
	{
		m_nBufferLength = 0;
	}
	else
	{
		m_nBufferLength = GetEncodedStreamLength(m_strValue);
	}
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(int nValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_INT;
	m_nValue = nValue;
	//m_strValue = wxString::Format(_("%d"), nValue);
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(double dblValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_DOUBLE;
	m_dblValue = dblValue;
	//m_strValue = wxString::Format(_("%f"), dblValue);
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(const ibNumber &numValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_DOUBLE;
	m_dblValue = numValue.ToDouble();
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(bool bValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_BOOL;
	m_bValue = bValue;
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(const wxDateTime& dateValue)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_DATETIME;

	m_pDate = new MYSQL_TIME();

	memset(m_pDate, 0, sizeof(MYSQL_TIME));

	m_pDate->year = dateValue.GetYear();
	m_pDate->month = dateValue.GetMonth() + 1;
	m_pDate->day = dateValue.GetDay();
	m_pDate->hour = dateValue.GetHour();
	m_pDate->minute = dateValue.GetMinute();
	m_pDate->second = dateValue.GetSecond();
	m_pDate->neg = 0;

	m_nBufferLength = sizeof(MYSQL_TIME);
}

ibDatabaseParameterMySQL::ibDatabaseParameterMySQL(const void* pData, long nDataLength)
{
	m_nParameterType = ibDatabaseParameterMySQL::PARAM_BLOB;
	void* pBuffer = m_BufferValue.GetWriteBuf(nDataLength);
	memcpy(pBuffer, pData, nDataLength);
	m_nBufferLength = nDataLength;
}

ibDatabaseParameterMySQL::~ibDatabaseParameterMySQL()
{
	if ((m_nParameterType == ibDatabaseParameterMySQL::PARAM_DATETIME) && (m_pDate != nullptr))
	{
		delete m_pDate;
		m_pDate = nullptr;
	}
}

long unsigned int ibDatabaseParameterMySQL::GetDataLength()
{
	return m_nBufferLength;
}

long unsigned int* ibDatabaseParameterMySQL::GetDataLengthPtr()
{
	return &m_nBufferLength;
}

const void* ibDatabaseParameterMySQL::GetDataPtr()
{
	const void *pReturn = nullptr;

	switch (m_nParameterType)
	{
	case ibDatabaseParameterMySQL::PARAM_STRING:
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterMySQL::PARAM_INT:
		pReturn = &m_nValue;
		break;
	case ibDatabaseParameterMySQL::PARAM_DOUBLE:
		pReturn = &m_dblValue;
		break;
	case ibDatabaseParameterMySQL::PARAM_DATETIME:
		pReturn = m_pDate;
		break;
	case ibDatabaseParameterMySQL::PARAM_BOOL:
		pReturn = &m_bValue;
		break;
	case ibDatabaseParameterMySQL::PARAM_BLOB:
		pReturn = m_BufferValue.GetData();
		break;
	case ibDatabaseParameterMySQL::PARAM_NULL:
		pReturn = nullptr;
		break;
	default:
		pReturn = nullptr;
		break;
	};
	return pReturn;
}

int ibDatabaseParameterMySQL::GetParameterType()
{
	return m_nParameterType;
}

enum_field_types ibDatabaseParameterMySQL::GetBufferType()
{
	enum_field_types returnType = MYSQL_TYPE_NULL;

	switch (m_nParameterType)
	{
	case ibDatabaseParameterMySQL::PARAM_STRING:
		returnType = MYSQL_TYPE_VAR_STRING;
		break;
	case ibDatabaseParameterMySQL::PARAM_INT:
		returnType = MYSQL_TYPE_LONG;
		break;
	case ibDatabaseParameterMySQL::PARAM_DOUBLE:
		returnType = MYSQL_TYPE_DOUBLE;
		break;
	case ibDatabaseParameterMySQL::PARAM_DATETIME:
		returnType = MYSQL_TYPE_TIMESTAMP;
		break;
	case ibDatabaseParameterMySQL::PARAM_BOOL:
		returnType = MYSQL_TYPE_TINY;
		break;
	case ibDatabaseParameterMySQL::PARAM_BLOB:
		returnType = MYSQL_TYPE_LONG_BLOB;
		break;
	case ibDatabaseParameterMySQL::PARAM_NULL:
		returnType = MYSQL_TYPE_NULL;
		break;
	default:
		returnType = MYSQL_TYPE_NULL;
		break;
	};

	return returnType;
}

