////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value DatabaseLayer 
////////////////////////////////////////////////////////////////////////////

#include "valueDatabase.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CValueDatabaseLayer, CValue);

CValue::CMethodHelper CValueDatabaseLayer::m_methodHelper;

enum
{
	ePrepareStatement,
	eRunQuery,
	eRunQueryWithResults,
};

CValueDatabaseLayer::CValueDatabaseLayer() :
	CValue(eValueTypes::TYPE_VALUE)
{
}

CValueDatabaseLayer::~CValueDatabaseLayer()
{
}

void CValueDatabaseLayer::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendFunc(wxT("prepareStatement"), 1, wxT("prepareStatement(string: query, ...)"));
	m_methodHelper.AppendFunc(wxT("runQuery"), 1, wxT("runQuery(string: query, ...)"));
	m_methodHelper.AppendFunc(wxT("runQueryWithResults"), 1, wxT("runQueryWithResults(string: query, ...)"));
}

#include "backend/backend_exception.h"

bool CValueDatabaseLayer::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) //function call
{
	if (lMethodNum == ePrepareStatement)
	{
		if (!appData->DesignerMode())
		{
			IPreparedStatement* preparedStatement = db_query->PrepareStatement(paParams[0]->GetString());
			if (preparedStatement == nullptr) {
				CBackendCoreException::Error(db_query->GetErrorMessage());
				return false;
			}
			pvarRetValue = CValue::CreateAndPrepareValueRef<CValuePreparedStatement>(preparedStatement);
			return true;
		}

		pvarRetValue = CValue::CreateAndPrepareValueRef<CValuePreparedStatement>();
		return true;
	}
	else if (lMethodNum == eRunQuery)
	{
		if (!appData->DesignerMode())
			pvarRetValue = db_query->RunQuery(paParams[0]->GetString());
		return true;
	}
	else if (lMethodNum == eRunQueryWithResults)
	{
		if (!appData->DesignerMode())
		{
			IDatabaseResultSet* resultSet = db_query->RunQueryWithResults(paParams[0]->GetString());
			if (resultSet == nullptr) {
				CBackendCoreException::Error(db_query->GetErrorMessage());
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

bool CValueDatabaseLayer::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray) //procudre call
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueDatabaseLayer, "databaseLayer", string_to_clsid("VL_DBLY"));