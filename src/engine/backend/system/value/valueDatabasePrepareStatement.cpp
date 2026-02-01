#include "valueDatabase.h"
#include "backend/databaseLayer/databaseLayer.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValuePreparedStatement, CValue);

CValue::CMethodHelper CValuePreparedStatement::m_methodHelper;

enum
{
	eSetParam,
	eRunQuery,
	eRunQueryWithResults,
};

CValuePreparedStatement::CValuePreparedStatement(IPreparedStatement* preparedStatement) : 
	CValue(eValueTypes::TYPE_VALUE), m_preparedStatement(preparedStatement) 
{
}

CValuePreparedStatement::~CValuePreparedStatement()
{
	if (m_preparedStatement != nullptr)
		m_preparedStatement->Close();

	wxDELETE(m_preparedStatement);
}
void CValuePreparedStatement::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendProc(wxT("setParam"), 2, wxT("runQuery(number: position, any:value)"));

	m_methodHelper.AppendFunc(wxT("runQuery"), 1, wxT("runQuery()"));
	m_methodHelper.AppendFunc(wxT("runQueryWithResults"), 1, wxT("runQueryWithResults()"));
}

#include "backend/backend_exception.h"

bool CValuePreparedStatement::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) //function call
{
	if (lMethodNum == eRunQuery)
	{
		if (m_preparedStatement != nullptr)
			pvarRetValue = m_preparedStatement->RunQuery();
		return true;
	}
	else if (lMethodNum == eRunQueryWithResults)
	{
		if (m_preparedStatement != nullptr) {
			IDatabaseResultSet* resultSet = m_preparedStatement->RunQueryWithResults();
			if (resultSet == nullptr) {
				CBackendCoreException::Error(m_preparedStatement->GetErrorMessage());
				return false;
			}
			pvarRetValue = CValue::CreateAndPrepareValueRef<CValueResultSet>(resultSet);
			return true;
		}

		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueResultSet>();
		return true;
	}

	return false;
}

bool CValuePreparedStatement::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray) //procudre call
{
	if (m_preparedStatement != nullptr && lMethodNum == eSetParam )
	{
		if (paParams[1]->GetType() == eValueTypes::TYPE_BOOLEAN)
			m_preparedStatement->SetParamBool(paParams[0]->GetInteger(), paParams[1]->GetBoolean());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_NUMBER)
			m_preparedStatement->SetParamNumber(paParams[0]->GetInteger(), paParams[1]->GetNumber());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_DATE)
			m_preparedStatement->SetParamDate(paParams[0]->GetInteger(), paParams[1]->GetDateTime());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_STRING)
			m_preparedStatement->SetParamDate(paParams[0]->GetInteger(), paParams[1]->GetDateTime());
		else 
			m_preparedStatement->SetParamNull(paParams[0]->GetInteger());

		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValuePreparedStatement, "databasePreparedStatement", string_to_clsid("VL_DBPS"));