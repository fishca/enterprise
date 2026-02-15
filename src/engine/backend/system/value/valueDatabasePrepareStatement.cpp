#include "valueDatabase.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

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
		db_query->CloseStatement(m_preparedStatement);
}

void CValuePreparedStatement::PrepareNames() const
{
	m_methodHelper.ClearHelper();
	m_methodHelper.AppendProc(wxT("SetParam"), 2, wxT("SetParam(number: position, any: value)"));

	m_methodHelper.AppendFunc(wxT("RunQuery"), 1, wxT("RunQuery()"));
	m_methodHelper.AppendFunc(wxT("RunQueryWithResults"), 1, wxT("RunQueryWithResults()"));
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
				CBackendCoreException::Error(CBackendCoreException::GetLastError());
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
	if (m_preparedStatement != nullptr && lMethodNum == eSetParam)
	{
		const int position = paParams[0]->GetInteger();
		if (position == 0 || position > m_preparedStatement->GetParameterCount()) {
			CBackendCoreException::Error(_("Index goes beyond statement"));
			return false;
		}

		if (paParams[1]->GetType() == eValueTypes::TYPE_BOOLEAN)
			m_preparedStatement->SetParamBool(position, paParams[1]->GetBoolean());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_NUMBER)
			m_preparedStatement->SetParamNumber(position, paParams[1]->GetNumber());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_DATE)
			m_preparedStatement->SetParamDate(position, paParams[1]->GetDateTime());
		else if (paParams[1]->GetType() == eValueTypes::TYPE_STRING)
			m_preparedStatement->SetParamString(position, paParams[1]->GetString());
		else
			m_preparedStatement->SetParamNull(position);

		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValuePreparedStatement, "DatabasePreparedStatement", string_to_clsid("VL_DBPS"));