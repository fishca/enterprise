#include "valueDatabase.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/appData.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueResultSet, CValue);

enum
{
	eNext,
};

CValueResultSet::CValueResultSet(IDatabaseResultSet* resultSet)
	: CValue(eValueTypes::TYPE_VALUE), m_resultSet(resultSet), m_methodHelper(new CMethodHelper)
{
}

CValueResultSet::~CValueResultSet()
{
	if (m_resultSet != nullptr)
		db_query->CloseResultSet(m_resultSet);

	wxDELETE(m_methodHelper);
}

void CValueResultSet::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc(wxT("Next"), wxT("Next()"));

	if (m_resultSet != nullptr) {
		IResultSetMetaData* resultSetMetaData = m_resultSet->GetMetaData();
		if (resultSetMetaData == nullptr)
			return;
		for (int idx = 1; idx <= resultSetMetaData->GetColumnCount(); idx++)
			m_methodHelper->AppendProp(resultSetMetaData->GetColumnName(idx), true, false, idx);
	}
}

bool CValueResultSet::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool CValueResultSet::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	IResultSetMetaData* resultSetMetaData = m_resultSet->GetMetaData();
	if (resultSetMetaData != nullptr) {

		const int columnFld = m_methodHelper->GetPropData(lPropNum);
		const int columnType = resultSetMetaData->GetColumnType(columnFld);
		
		if (columnType == IResultSetMetaData::COLUMN_NULL)
			pvarPropVal = CValue::CreateObject(g_valueNullCLSID);
		else if (columnType == IResultSetMetaData::COLUMN_BOOL)
			pvarPropVal = m_resultSet->GetResultBool(columnFld);
		else if (columnType == IResultSetMetaData::COLUMN_INTEGER || columnType == IResultSetMetaData::COLUMN_DOUBLE)
			pvarPropVal = m_resultSet->GetResultNumber(columnFld);
		else if (columnType == IResultSetMetaData::COLUMN_DATE)
			pvarPropVal = m_resultSet->GetResultDate(columnFld);
		else if (columnType == IResultSetMetaData::COLUMN_STRING)
			pvarPropVal = m_resultSet->GetResultString(columnFld);

		return true;
	}

	return false;
}

bool CValueResultSet::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray) //function call
{
	if (m_resultSet != nullptr && lMethodNum == eNext) {
		pvarRetValue = m_resultSet->Next();
		return true;
	}

	return false;
}

bool CValueResultSet::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray) //procudre call
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValueResultSet, "DatabaseResultSet", string_to_clsid("VL_DBRS"));