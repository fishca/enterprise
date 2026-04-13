#include "mysqlPreparedStatement.h"
#include "mysqlDatabaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

ibPreparedStatementMySQL::ibPreparedStatementMySQL(ibInterfaceMySQL* pInterface)
	: ibPreparedStatement()
{
	m_pInterface = pInterface;
	m_Statements.clear();
}

ibPreparedStatementMySQL::ibPreparedStatementMySQL(ibInterfaceMySQL* pInterface, MYSQL_STMT* pStatement)
	: ibPreparedStatement()
{
	m_pInterface = pInterface;
	AddPreparedStatement(pStatement);
}

ibPreparedStatementMySQL::~ibPreparedStatementMySQL()
{
	Close();
}


void ibPreparedStatementMySQL::Close()
{
	CloseResultSets();

	// Free the statements
	MysqlStatementWrapperArray::iterator start = m_Statements.begin();
	MysqlStatementWrapperArray::iterator stop = m_Statements.end();

	while (start != stop)
	{
		if ((*start) != nullptr)
		{
			ibPreparedStatementMySQLWrapper* pWrapper = (ibPreparedStatementMySQLWrapper*)(*start);
			wxDELETE(pWrapper);
			(*start) = nullptr;
		}
		start++;
	}
}

void ibPreparedStatementMySQL::AddPreparedStatement(MYSQL_STMT* pStatement)
{
	ibPreparedStatementMySQLWrapper* pStatementWrapper = new ibPreparedStatementMySQLWrapper(m_pInterface, pStatement);
	if (pStatementWrapper)
		pStatementWrapper->SetEncoding(GetEncoding());
	m_Statements.push_back(pStatementWrapper);
}

// get field
void ibPreparedStatementMySQL::SetParamInt(int nPosition, int nValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, nValue);
	}
}

void ibPreparedStatementMySQL::SetParamDouble(int nPosition, double dblValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, dblValue);
	}
}

void ibPreparedStatementMySQL::SetParamNumber(int nPosition, const ibNumber &numValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, numValue);
	}
}

void ibPreparedStatementMySQL::SetParamString(int nPosition, const wxString& strValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, strValue);
	}
}

void ibPreparedStatementMySQL::SetParamNull(int nPosition)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition);
	}
}

void ibPreparedStatementMySQL::SetParamBlob(int nPosition, const void* pData, long nDataLength)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, pData, nDataLength);
	}
}

void ibPreparedStatementMySQL::SetParamDate(int nPosition, const wxDateTime& dateValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, dateValue);
	}
}

void ibPreparedStatementMySQL::SetParamBool(int nPosition, bool bValue)
{
	int nIndex = FindStatementAndAdjustPositionIndex(&nPosition);
	if (nIndex > -1)
	{
		m_Statements[nIndex]->SetParam(nPosition, bValue);
	}
}

int ibPreparedStatementMySQL::GetParameterCount()
{
	MysqlStatementWrapperArray::iterator start = m_Statements.begin();
	MysqlStatementWrapperArray::iterator stop = m_Statements.end();

	int nParameters = 0;
	while (start != stop)
	{
		nParameters += ((ibPreparedStatementMySQLWrapper*)(*start))->GetParameterCount();
		start++;
	}
	return nParameters;
}

int ibPreparedStatementMySQL::RunQuery()
{
	MysqlStatementWrapperArray::iterator start = m_Statements.begin();
	MysqlStatementWrapperArray::iterator stop = m_Statements.end();

	int nRows = -1;
	while (start != stop)
	{
		nRows = ((ibPreparedStatementMySQLWrapper*)(*start))->DoRunQuery();
		if (((ibPreparedStatementMySQLWrapper*)(*start))->GetErrorCode() != DATABASE_LAYER_OK)
		{
			SetErrorCode(((ibPreparedStatementMySQLWrapper*)(*start))->GetErrorCode());
			SetErrorMessage(((ibPreparedStatementMySQLWrapper*)(*start))->GetErrorMessage());
			ThrowDatabaseException();
			return DATABASE_LAYER_QUERY_RESULT_ERROR;
		}
		start++;
	}
	return nRows;
}

ibDatabaseResultSet* ibPreparedStatementMySQL::RunQueryWithResults()
{
	if (m_Statements.size() > 0)
	{
		for (unsigned int i = 0; i < (m_Statements.size() - 1); i++)
		{
			ibPreparedStatementMySQLWrapper* pStatement = m_Statements[i];
			pStatement->DoRunQuery();
			if (pStatement->GetErrorCode() != DATABASE_LAYER_OK)
			{
				SetErrorCode(pStatement->GetErrorCode());
				SetErrorMessage(pStatement->GetErrorMessage());
				ThrowDatabaseException();
				return nullptr;
			}
		}

		ibPreparedStatementMySQLWrapper* pLastStatement = m_Statements[m_Statements.size() - 1];
		ibDatabaseResultSet* pResults = pLastStatement->DoRunQueryWithResults();
		if (pLastStatement->GetErrorCode() != DATABASE_LAYER_OK)
		{
			SetErrorCode(pLastStatement->GetErrorCode());
			SetErrorMessage(pLastStatement->GetErrorMessage());
			ThrowDatabaseException();
		}
		LogResultSetForCleanup(pResults);
		return pResults;
	}
	else
		return nullptr;
}

int ibPreparedStatementMySQL::FindStatementAndAdjustPositionIndex(int* pPosition)
{
	if (m_Statements.size() == 0)
		return 0;

	// Go through all the elements in the vector
	// Get the number of parameters in each statement
	// Adjust the nPosition for the the broken up statements
	for (unsigned int i = 0; i < m_Statements.size(); i++)
	{
		int nParametersInThisStatement = m_Statements[i]->GetParameterCount();

		if (*pPosition > nParametersInThisStatement)
		{
			*pPosition -= nParametersInThisStatement;    // Decrement the position indicator by the number of parameters in this statement
		}
		else
		{
			// We're in the correct statement, return the index
			return i;
		}
	}
	return -1;
}

