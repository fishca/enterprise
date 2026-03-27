////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value table and key pair 
////////////////////////////////////////////////////////////////////////////

#include "valueTable.h"
#include "backend/backend_exception.h"

//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTableMemory, ibValueModelTable);

//////////////////////////////////////////////////////////////////////
ibValue::ibValueMethodHelper ibValueModelTableMemory::m_methodHelper;
//////////////////////////////////////////////////////////////////////

ibDataViewItem ibValueModelTableMemory::FindRowValue(const ibValue& varValue, const wxString& colName) const
{
	ibValueModelColumnCollection::ibValueModelColumnInfo* colInfo = m_tableColumnCollection->GetColumnByName(colName);
	if (colInfo != nullptr) {
		for (long row = 0; row < GetRowCount(); row++) {
			const ibDataViewItem& item = GetItem(row);
			wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
			if (node != nullptr &&
				varValue == node->GetTableValue((ibMetaID)colInfo->GetColumnID())) {
				return item;
			}
		}
	}
	return ibDataViewItem(nullptr);
}

ibDataViewItem ibValueModelTableMemory::FindRowValue(ibValueModelReturnLine* retLine) const
{
	return ibDataViewItem(nullptr);
}

ibValueModelTableMemory::ibValueModelTableMemory() : ibValueModelTable(),
m_tableColumnCollection(ibValue::CreateAndPrepareValueRef<ibValueModelTableColumnCollection>(this))
{
}

ibValueModelTableMemory::ibValueModelTableMemory(const ibValueModelTableMemory& valueTable) : ibValueModelTable(),
m_tableColumnCollection(valueTable.m_tableColumnCollection)
{
}

ibValueModelTableMemory::~ibValueModelTableMemory()
{
}

void ibValueModelTableMemory::PrepareNames() const
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

bool ibValueModelTableMemory::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enColumns:
		pvarPropVal = m_tableColumnCollection;
		return true;
	}

	return false;
}

bool ibValueModelTableMemory::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
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
		const ibDataViewItem& item = FindRowValue(*paParams[0], paParams[1]->GetString());
		if (item.IsOk())
			pvarRetValue = GetRowAt(item);
		return true;
	}
	case enCount:
		pvarRetValue = (unsigned int)GetRowCount();
		return true;
	case enDelete:
	{
		ibValueModelTableReturnLine* retLine = nullptr;
		if (paParams[0]->ConvertToValue(retLine)) {
			wxValueTableRow* node = GetViewData<wxValueTableRow>(retLine->GetLineItem());
			if (node != nullptr)
				ibValueModelTable::Remove(node);
		}
		else {
			wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(paParams[0]->GetInteger()));
			if (node != nullptr) ibValueModelTable::Remove(node);
		}
		return true;
	}
	case enClear:
		Clear();
		return true;
	case enSort:
		ibValueModelColumnCollection::ibValueModelColumnInfo* colInfo = m_tableColumnCollection->GetColumnByName(paParams[0]->GetString());
		if (colInfo != nullptr) {
			ibValueModelTable::Sort(colInfo->GetColumnID(), lSizeArray > 0 ? paParams[1]->GetBoolean() : true);
			return true;
		}
		return false;
	}

	return false;
}

#include "backend/appData.h"

bool ibValueModelTableMemory::GetAt(const ibValue& varKeyValue, ibValue& pvarValue)
{
	const long index = varKeyValue.GetUInteger();
	if (index >= GetRowCount() && !appData->DesignerMode()) {
		ibBackendCoreException::Error(_("Array index out of bounds"));
		return false;
	}
	pvarValue = ibValue::CreateAndPrepareValueRef<ibValueModelTableReturnLine>(this, GetItem(index));
	return true;
}

//////////////////////////////////////////////////////////////////////
//               ibValueModelTableColumnCollection                        //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTableMemory::ibValueModelTableColumnCollection, ibValueModelTable::ibValueModelColumnCollection);

ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnCollection(ibValueModelTableMemory* ownerTable) : ibValueModelColumnCollection(),
m_ownerTable(ownerTable),
m_methodHelper(new ibValueMethodHelper())
{
}

ibValueModelTableMemory::ibValueModelTableColumnCollection::~ibValueModelTableColumnCollection() {
	wxDELETE(m_methodHelper);
}

//работа с массивом как с агрегатным объектом
//перечисление строковых ключей
void ibValueModelTableMemory::ibValueModelTableColumnCollection::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc(wxT("AddColumn"), 4, wxT("Add(name : string, type : typeDescription, caption, width)"));
	m_methodHelper->AppendProc(wxT("RemoveColumn"), 1, wxT("RemoveColumn(name : string)"));
}

#include "valueType.h"

bool ibValueModelTableMemory::ibValueModelTableColumnCollection::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRemoveColumn:
	{
		wxString columnName = paParams[0]->GetString();
		auto it = std::find_if(m_listColumnInfo.begin(), m_listColumnInfo.end(),
			[columnName](ibValueModelTableColumnInfo* colData)
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

bool ibValueModelTableMemory::ibValueModelTableColumnCollection::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enAddColumn:
	{
		ibValueType* valueType = nullptr;
		if (lSizeArray > 1)
			paParams[1]->ConvertToValue(valueType);
		if (lSizeArray > 3)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<ibValueTypeDescription>(), paParams[2]->GetString(), paParams[3]->GetInteger());
		else if (lSizeArray > 2)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<ibValueTypeDescription>(), paParams[2]->GetString(), wxDVC_DEFAULT_WIDTH);
		else if (lSizeArray > 1)
			pvarRetValue = AddColumn(paParams[0]->GetString(), valueType ? valueType->GetOwnerTypeDescription() : *paParams[1]->ConvertToType<ibValueTypeDescription>(), paParams[0]->GetString(), wxDVC_DEFAULT_WIDTH);
		else
			pvarRetValue = AddColumn(paParams[0]->GetString(), ibTypeDescription(g_valueStringCLSID), paParams[0]->GetString(), wxDVC_DEFAULT_WIDTH);
		return true;
	}
	}

	return false;
}

bool ibValueModelTableMemory::ibValueModelTableColumnCollection::SetAt(const ibValue& varKeyValue, const ibValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool ibValueModelTableMemory::ibValueModelTableColumnCollection::GetAt(const ibValue& varKeyValue, ibValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		ibBackendCoreException::Error(_("Index goes beyond array")); 
		return false;
	}
	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = *it;
	return true;
}

//////////////////////////////////////////////////////////////////////
//               ibValueModelTableColumnInfo                              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnInfo, ibValueModelTable::ibValueModelColumnCollection::ibValueModelColumnInfo);

ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnInfo::ibValueModelTableColumnInfo() : ibValueModelColumnInfo() {
}

ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnInfo::ibValueModelTableColumnInfo(unsigned int colID, const wxString& colName, const ibTypeDescription& typeDescription, const wxString& caption, int width) :
	ibValueModelColumnInfo(), m_columnID(colID), m_columnName(colName), m_columnType(typeDescription), m_columnCaption(caption), m_columnWidth(width) {
}

ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnInfo::~ibValueModelTableColumnInfo() {
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//               ibValueModelTableReturnLine                              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTableMemory::ibValueModelTableReturnLine, ibValueModelTable::ibValueModelReturnLine);

ibValueModelTableMemory::ibValueModelTableReturnLine::ibValueModelTableReturnLine(ibValueModelTableMemory* ownerTable, const ibDataViewItem& line) :
	ibValueModelReturnLine(line), m_methodHelper(new ibValueMethodHelper()), m_ownerTable(ownerTable) {
}

ibValueModelTableMemory::ibValueModelTableReturnLine::~ibValueModelTableReturnLine() {
	wxDELETE(m_methodHelper);
}

void ibValueModelTableMemory::ibValueModelTableReturnLine::PrepareNames() const
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

bool ibValueModelTableMemory::ibValueModelTableReturnLine::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (appData->DesignerMode())
		return false;
	return SetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum),
		varPropVal
	);
}

bool ibValueModelTableMemory::ibValueModelTableReturnLine::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;

	return GetValueByMetaID(
		m_methodHelper->GetPropData(lPropNum), pvarPropVal
	);
}

//**********************************************************************

long ibValueModelTableMemory::AppendRow(unsigned int before)
{
	wxValueTableRow* rowData = new wxValueTableRow();
	for (auto& colData : m_tableColumnCollection->m_listColumnInfo) {
		rowData->AppendTableValue(colData->GetColumnID(),
			ibValueTypeDescription::AdjustValue(m_tableColumnCollection->GetColumnType(colData->GetColumnID()))
		);
	}

	return ibValueModelTable::Append(rowData, !ibBackendException::IsEvalMode());
}

void ibValueModelTableMemory::EditRow()
{
	ibValueModelTable::RowValueStartEdit(GetSelection());
}

void ibValueModelTableMemory::CopyRow()
{
	ibDataViewItem currentItem = GetSelection();
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
		ibValueModelTable::Insert(rowData, currentLine, !ibBackendException::IsEvalMode());
	}
	else {
		ibValueModelTable::Append(rowData, !ibBackendException::IsEvalMode());
	}
}

void ibValueModelTableMemory::DeleteRow()
{
	ibDataViewItem currentItem = GetSelection();
	if (!currentItem.IsOk())
		return;
	wxValueTableRow* node = GetViewData<wxValueTableRow>(currentItem);
	if (node == nullptr)
		return;
	if (!ibBackendException::IsEvalMode())
		ibValueModelTable::Remove(node);
}

void ibValueModelTableMemory::Clear()
{
	if (ibBackendException::IsEvalMode())
		return;
	ibValueModelTable::Clear();
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(ibValueModelTableMemory, "Table", g_valueTableCLSID);

SYSTEM_TYPE_REGISTER(ibValueModelTableMemory::ibValueModelTableColumnCollection, "TableValueColumn", string_to_clsid("VL_TVCLM"));
SYSTEM_TYPE_REGISTER(ibValueModelTableMemory::ibValueModelTableColumnCollection::ibValueModelTableColumnInfo, "TableValueColumnInfo", string_to_clsid("VL_TVCLI"));
SYSTEM_TYPE_REGISTER(ibValueModelTableMemory::ibValueModelTableReturnLine, "TableValueRow", string_to_clsid("VL_TVROW"));