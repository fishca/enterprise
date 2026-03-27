#include "postgresParameter.h"
#include "backend/databaseLayer/databaseLayer.h"

// ctor
ibDatabaseParameterPostgres::ibDatabaseParameterPostgres() : m_nParameterType(ibDatabaseParameterPostgres::PARAM_NULL)
{
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(const wxString& strValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_STRING), m_strValue(strValue), m_nBufferLength(strValue.Length())
{
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(int nValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_INT)
{
	m_strValue = wxString::Format(wxT("%d"), nValue);
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(double dblValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_DOUBLE)
{
	m_strValue = wxString::Format(wxT("%f"), dblValue);
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(const ibNumber &dblValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_NUMBER)
{
	m_strValue = dblValue.ToWString();
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(bool bValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_BOOL)
{
	m_strValue = wxString::Format(wxT("%d"), bValue);
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(const wxDateTime& dateValue) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_DATETIME)
{
	m_strDateValue = dateValue.Format(wxT("%Y-%m-%d %H:%M:%S"));
	m_nBufferLength = m_strDateValue.Length();
}

ibDatabaseParameterPostgres::ibDatabaseParameterPostgres(const void* pData, long nDataLength) : m_nParameterType(ibDatabaseParameterPostgres::PARAM_BLOB)
{
	void* pBuffer = m_BufferValue.GetWriteBuf(nDataLength);
	memcpy(pBuffer, pData, nDataLength);
	m_nBufferLength = nDataLength;
}

long ibDatabaseParameterPostgres::GetDataLength()
{
	return m_nBufferLength;
}

long* ibDatabaseParameterPostgres::GetDataLengthPointer()
{
	return &m_nBufferLength;
}

const void* ibDatabaseParameterPostgres::GetDataPtr()
{
	const void *pReturn = nullptr;

	switch (m_nParameterType)
	{
	case ibDatabaseParameterPostgres::PARAM_STRING:
		m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_INT:
		m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_DOUBLE:
		m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_NUMBER:
		m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_DATETIME:
		m_CharBufferValue = ConvertToUnicodeStream(m_strDateValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_BOOL:
		m_CharBufferValue = ConvertToUnicodeStream(m_strValue);
		pReturn = m_CharBufferValue;
		break;
	case ibDatabaseParameterPostgres::PARAM_BLOB:
		pReturn = m_BufferValue.GetData();
		break;
	case ibDatabaseParameterPostgres::PARAM_NULL:
		pReturn = nullptr;
		break;
	default:
		pReturn = nullptr;
		break;
	};
	
	return pReturn;
}

int ibDatabaseParameterPostgres::GetParameterType()
{
	return m_nParameterType;
}

bool ibDatabaseParameterPostgres::IsBinary()
{
	return (ibDatabaseParameterPostgres::PARAM_BLOB == m_nParameterType);
}

