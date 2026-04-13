#include "mysqlPreparedStatementParameterCollection.h"

ibPreparedStatementMySQLParameterCollection::ibPreparedStatementMySQLParameterCollection()
{
}

ibPreparedStatementMySQLParameterCollection::~ibPreparedStatementMySQLParameterCollection()
{
	MysqlParameterArray::iterator start = m_Parameters.begin();
	MysqlParameterArray::iterator stop = m_Parameters.end();

	while (start != stop)
	{
		if ((*start) != nullptr)
		{
			ibDatabaseParameterMySQL* pParameter = (ibDatabaseParameterMySQL*)(*start);
			wxDELETE(pParameter);
			(*start) = nullptr;
		}
		start++;
	}
}

int ibPreparedStatementMySQLParameterCollection::GetSize()
{
	return m_Parameters.size();
}

MYSQL_BIND* ibPreparedStatementMySQLParameterCollection::GetMysqlParameterBindings()
{
	MYSQL_BIND* pBindings = new MYSQL_BIND[m_Parameters.size()];

	memset(pBindings, 0, sizeof(MYSQL_BIND)*m_Parameters.size());

	for (unsigned int i = 0; i < m_Parameters.size(); i++)
	{
		pBindings[i].buffer_type = m_Parameters[i]->GetBufferType();
		pBindings[i].buffer = (void*)m_Parameters[i]->GetDataPtr();
		pBindings[i].buffer_length = m_Parameters[i]->GetDataLength();
		pBindings[i].length = m_Parameters[i]->GetDataLengthPtr();
	}

	return pBindings;
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, int nValue)
{
	//ibDatabaseParameterMySQL Parameter(nValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(nValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, double dblValue)
{
	//ibDatabaseParameterMySQL Parameter(dblValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(dblValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, const ibNumber &numValue)
{
	//ibDatabaseParameterMySQL Parameter(dblValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(numValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, const wxString& strValue)
{
	//ibDatabaseParameterMySQL Parameter(strValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(strValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition)
{
	//ibDatabaseParameterMySQL Parameter;
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL();
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, const void* pData, long nDataLength)
{
	//ibDatabaseParameterMySQL Parameter(pData, nDataLength);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(pData, nDataLength);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, const wxDateTime& dateValue)
{
	//ibDatabaseParameterMySQL Parameter(dateValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(dateValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, bool bValue)
{
	//ibDatabaseParameterMySQL Parameter(bValue);
	//SetParam(nPosition, Parameter);
	ibDatabaseParameterMySQL* pParameter = new ibDatabaseParameterMySQL(bValue);
	pParameter->SetEncoding(GetEncoding());
	SetParam(nPosition, pParameter);
}

void ibPreparedStatementMySQLParameterCollection::SetParam(int nPosition, ibDatabaseParameterMySQL* pParameter)
{
	// First make sure that there are enough elements in the collection
	while (m_Parameters.size() < (unsigned int)(nPosition))
	{
		//ibDatabaseParameterMySQL EmptyParameter;
		m_Parameters.push_back(nullptr);//EmptyParameter);
	}
	// Free up any data that is being replaced so the allocated memory isn't lost
	if (m_Parameters[nPosition - 1] != nullptr)
	{
		delete (m_Parameters[nPosition - 1]);
	}
	// Now set the new data
	m_Parameters[nPosition - 1] = pParameter;
}

