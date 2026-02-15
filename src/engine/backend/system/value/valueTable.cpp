////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value table and key pair 
////////////////////////////////////////////////////////////////////////////

#include "valueTable.h"
#include "backend/backend_exception.h"

//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableMemory, IValueTable);

//////////////////////////////////////////////////////////////////////
CValue::CMethodHelper CValueTableMemory::m_methodHelper;
//////////////////////////////////////////////////////////////////////

wxDataViewItem CValueTableMemory::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	IValueModelColumnCollection::IValueModelColumnInfo* colInfo = m_tableColumnCollection->GetColumnByName(colName);
	if (colInfo != nullptr) {
		for (long row = 0; row < GetRowCount(); row++) {
			const wxDataViewItem& item = GetItem(row);
			wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
			if (node != nullptr &&
				varValue == node->GetTableValue((meta_identifier_t)colInfo->GetColumnID())) {
				return item;
			}
		}
	}
	return wxDataViewItem(nullptr);
}

wxDataViewItem CValueTableMemory::FindRowValue(IValueModelReturnLine* retLine) const
{
	return wxDataViewItem(nullptr);
}

CValueTableMemory::CValueTableMemory() : IValueTable(),
m_tableColumnCollection(CValue::CreateAndPrepareValueRef<CValueTableColumnCollection>(this))
{
}

CValueTableMemory::CValueTableMemory(const CValueTableMemory& valueTable) : IValueTable(),
m_tableColumnCollection(valueTable.m_tableColumnCollection)
{
}

CValueTableMemory::~CValueTableMemory()
{
}

void CValueTableMemory::PrepareNames() const
{
	m_methodHelper.ClearHelper();

	m_methodHelper.AppendFunc(wxT("Add"), wxT("Add()"));
	m_methodHelper.AppendFunc(wxT("Clone"), wxT("Clone()"));
	m_methodHelper.AppendFunc(wxT("Count"), wxT("Count()"));
	m_methodHelper.AppendFunc(wxT("Find"), 2, wxT("Find(value : any, column : string)"));
	m_methodHelper.AppendFunc(wxT("Delete"), 1, wxT("Delete(row : tableRow)"));
	m_methodHelper.AppendFunc(wxT("Clear"), wxT("Clear()"));
	m_methodHelper.AppendFunc(wxT("Sort"), 2, wxT("Sort(column : string, ascending = true : boolean)"));

	m_methodHelper.AppendProp(wxT("Columns"));

	if (m_tableColumnCollection != nullptr)
		m_tableColumnCollection->PrepareNames();
}

bool CValueTableMemory::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enColumns:
		pvarPropVal = m_tableColumnCollection;
		return true;
	}

	return false;
}

bool CValueTableMemory::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enAddRow:
		pvarRetValue = GetRowAt(AppendRow());
		return true;
	case enClone:
		pvarRetValue = Clone();
		return true;
	case enFind:
	{
		const wxDataViewItem& item = FindRowValue(*paParams[0], paParams[1]->GetString());
		if (item.IsOk())
			pvarRetValue = GetRowAt(item);
		return true;
	}
	case enCount:
		pvarRetValue = (unsigned int)GetRowCount();
		return true;
	case enDelete:
	{
		CValueTableReturnLine* retLine = nullptr;
		if (paParams[0]->ConvertToValue(retLine)) {
			wxValueTableRow* node = GetViewData<wxValueTableRow>(retLine->GetLineItem());
			if (node != nullptr)
				IValueTable::Remove(node);
		}
		else {
			wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(paParams[0]->GetInteger()));
			if (node != nullptr) IValueTable::Remove(node);
		}
		return true;
	}
	case enClear:
		Clear();
		return true;
	case enSort:
		IValueModelColumnCollection::IValueModelColumnInfo* colInfo = m_tableColumnCollection->GetColumnByName(paParams[0]->GetString());
		if (colInfo != nullptr) {
			IValueTable::Sort(colInfo->GetColumnID(), lSizeArray > 0 ? paParams[1]->GetBoolean() : true);
			return true;
		}
		return false;
	}

	return false;
}

#include "backend/appData.h"

bool CValueTableMemory::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	const long index = varKeyValue.GetUInteger();
	if (index >= GetRowCount() && !appData->DesignerMode()) {
		CBackendCoreException::Error(_("Array index out of bounds"));
		return false;
	}
	pvarValue = CValue::CreateAndPrepareValueRef<CValueTableReturnLine>(this, GetItem(index));
	return true;
}

//////////////////////////////////////////////////////////////////////
//               CValueTableColumnCollection                        //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableMemory::CValueTableColumnCollection, IValueTable::IValueModelColumnCollection);

CValueTableMemory::CValueTableColumnCollection::CValueTableColumnCollection(CValueTableMemory* ownerTable) : IValueModelColumnCollection(),
m_ownerTable(ownerTable),
m_methodHelper(new CMethodHelper())
{
}

CValueTableMemory::CValueTableColumnCollection::~CValueTableColumnCollection() {
	wxDELETE(m_methodHelper);
}

//работа с массивом как с агрегатным объектом
//перечисление строковых ключей
void CValueTableMemory::CValueTableColumnCollection::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc(wxT("AddColumn"), 4, wxT("Add(name : string, type : typeDescription, caption, width)"));
	m_methodHelper->AppendProc(wxT("RemoveColumn"), 1, wxT("RemoveColumn(name : string)"));
}

#include "valueType.h"

bool CValueTableMemory::CValueTableColumnCollection::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRemoveColumn:
	{
		wxString columnName = paParams[0]->GetString();
		auto it = std::find_if(m_listColumnInfo.begin(), m_listColumnInfo.end(),
			[columnName](CValueTableColumnInfo* colData)
			{
				return stringUtils::CompareString(columnName, colData->GetColumnName());
			});
		if (it != m_listColumnInfo.end()) {
			RemoveColumn((*it)->GetColumnID());
		}
		return true;
	}
	}
	return false;
}

bool CValueTableMemory::CValueTableColumnCollection::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enAddColumn:
	{
		CValueType* valueType = nullptr;
		if (lSizeArray > 1)
			paParams[1]->ConvertToValue(valueType);
		if (lSizeArray > 3)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<CValueTypeDescription>(), paParams[2]->GetString(), paParams[3]->GetInteger());
		else if (lSizeArray > 2)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<CValueTypeDescription>(), paParams[2]->GetString(), wxDVC_DEFAULT_WIDTH);
		else if (lSizeArray > 1)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<CValueTypeDescription>(), paParams[0]->GetString(), wxDVC_DEFAULT_WIDTH);
		else
			pvarRetValue = AddColumn(paParams[0]->GetString(), CTypeDescription(g_valueStringCLSID), paParams[0]->GetString(), wxDVC_DEFAULT_WIDTH);
		return true;
	}
	}

	return false;
}

bool CValueTableMemory::CValueTableColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool CValueTableMemory::CValueTableColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendCoreException::Error(_("Index goes beyond array")); 
		return false;
	}
	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = *it;
	return true;
}

//////////////////////////////////////////////////////////////////////
//               CValueTableColumnInfo                              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableMemory::CValueTableColumnCollection::CValueTableColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

CValueTableMemory::CValueTableColumnCollection::CValueTableColumnInfo::CValueTableColumnInfo() : IValueModelColumnInfo() {
}

CValueTableMemory::CValueTableColumnCollection::CValueTableColumnInfo::CValueTableColumnInfo(unsigned int colID, const wxString& colName, const CTypeDescription& typeDescription, const wxString& caption, int width) :
	IValueModelColumnInfo(), m_columnID(colID), m_columnName(colName), m_columnType(typeDescription), m_columnCaption(caption), m_columnWidth(width) {
}

CValueTableMemory::CValueTableColumnCollection::CValueTableColumnInfo::~CValueTableColumnInfo() {
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//               CValueTableReturnLine                              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableMemory::CValueTableReturnLine, IValueTable::IValueModelReturnLine);

CValueTableMemory::CValueTableReturnLine::CValueTableReturnLine(CValueTableMemory* ownerTable, const wxDataViewItem& line) :
	IValueModelReturnLine(line), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable) {
}

CValueTableMemory::CValueTableReturnLine::~CValueTableReturnLine() {
	wxDELETE(m_methodHelper);
}

void CValueTableMemory::CValueTableReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	for (auto& colInfo : m_ownerTable->m_tableColumnCollection->m_listColumnInfo) {
		wxASSERT(colInfo);
		m_methodHelper->AppendProp(
			colInfo->GetColumnName(),
			colInfo->GetColumnID()
		);
	}
}

bool CValueTableMemory::CValueTableReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	if (appData->DesignerMode())
		return false;
	return SetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum),
		varPropVal
	);
}

bool CValueTableMemory::CValueTableReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;

	return GetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum), pvarPropVal
	);
}

//**********************************************************************

long CValueTableMemory::AppendRow(unsigned int before)
{
	wxValueTableRow* rowData = new wxValueTableRow();
	for (auto& colData : m_tableColumnCollection->m_listColumnInfo) {
		rowData->AppendTableValue(colData->GetColumnID(),
			CValueTypeDescription::AdjustValue(m_tableColumnCollection->GetColumnType(colData->GetColumnID()))
		);
	}

	return IValueTable::Append(rowData, !CBackendException::IsEvalMode());
}

void CValueTableMemory::EditRow()
{
	IValueTable::RowValueStartEdit(GetSelection());
}

void CValueTableMemory::CopyRow()
{
	wxDataViewItem currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;
	wxValueTableRow* rowData = new wxValueTableRow();
	for (auto& colData : m_tableColumnCollection->m_listColumnInfo) {
		rowData->AppendTableValue(
			colData->GetColumnID(), node->GetTableValue(colData->GetColumnID())
		);
	}
	const long& currentLine = GetRow(currentItem);
	if (currentLine != wxNOT_FOUND) {
		IValueTable::Insert(rowData, currentLine, !CBackendException::IsEvalMode());
	}
	else {
		IValueTable::Append(rowData, !CBackendException::IsEvalMode());
	}
}

void CValueTableMemory::DeleteRow()
{
	wxDataViewItem currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;
	if (!CBackendException::IsEvalMode())
		IValueTable::Remove(node);
}

void CValueTableMemory::Clear()
{
	if (CBackendException::IsEvalMode())
		return;
	IValueTable::Clear();
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValueTableMemory, "Table", g_valueTableCLSID);

SYSTEM_TYPE_REGISTER(CValueTableMemory::CValueTableColumnCollection, "TableValueColumn", string_to_clsid("VL_TVCLM"));
SYSTEM_TYPE_REGISTER(CValueTableMemory::CValueTableColumnCollection::CValueTableColumnInfo, "TableValueColumnInfo", string_to_clsid("VL_TVCLI"));
SYSTEM_TYPE_REGISTER(CValueTableMemory::CValueTableReturnLine, "TableValueRow", string_to_clsid("VL_TVROW"));