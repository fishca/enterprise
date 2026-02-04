////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : tabular sections
////////////////////////////////////////////////////////////////////////////

#include "tabularSection.h"

#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaCollection/partial/reference/reference.h"

#include "backend/appData.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueTabularSectionDataObject, IValueTable);
wxIMPLEMENT_DYNAMIC_CLASS(IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine, IValueTable::IValueModelReturnLine);
wxIMPLEMENT_DYNAMIC_CLASS(CValueTabularSectionDataObject, IValueTabularSectionDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(CValueTabularSectionDataObjectRef, IValueTabularSectionDataObject);

//////////////////////////////////////////////////////////////////////
//               IValueTabularSectionDataObject                          //
//////////////////////////////////////////////////////////////////////

#include "backend/metaData.h"
#include "backend/objCtor.h"

wxDataViewItem IValueTabularSectionDataObject::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	IValueModelColumnCollection::IValueModelColumnInfo* colInfo = m_recordColumnCollection->GetColumnByName(colName);
	if (colInfo != nullptr) {
		for (long row = 0; row < GetRowCount(); row++) {
			const wxDataViewItem& item = GetItem(row);
			wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
			if (node != nullptr &&
				varValue == node->GetTableValue(colInfo->GetColumnID())) {
				return item;
			}
		}
	}
	return wxDataViewItem(nullptr);
}

wxDataViewItem IValueTabularSectionDataObject::FindRowValue(IValueModelReturnLine* retLine) const
{
	return wxDataViewItem(nullptr);
}

bool IValueTabularSectionDataObject::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	long index = varKeyValue.GetUInteger();
	if (index >= GetRowCount() && !appData->DesignerMode()) {
		CBackendCoreException::Error(_("Array index out of bounds"));
		return false;
	}

	pvarValue = CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectReturnLine>(this, GetItem(index));
	return true;
}

class_identifier_t IValueTabularSectionDataObject::GetClassType() const
{
	const IMetaData* metaData = m_metaTable->GetMetaData();
	wxASSERT(metaData);
	if (m_metaTable->IsAllowed()) {
		const IMetaValueTypeCtor* clsFactory =
			metaData->GetTypeCtor(m_metaTable, eCtorMetaType::eCtorMetaType_TabularSection);
		wxASSERT(clsFactory);
		return clsFactory->GetClassType();
	}
	return 0;
}

wxString IValueTabularSectionDataObject::GetClassName() const
{
	const IMetaData* metaData = m_metaTable->GetMetaData();
	wxASSERT(metaData);
	if (m_metaTable->IsAllowed()) {
		const IMetaValueTypeCtor* clsFactory =
			metaData->GetTypeCtor(m_metaTable, eCtorMetaType::eCtorMetaType_TabularSection);
		wxASSERT(clsFactory);
		return clsFactory->GetClassName();
	}
	return _("<deleted metaobject>");
}

wxString IValueTabularSectionDataObject::GetString() const
{
	if (m_metaTable->IsAllowed()) {
		const IMetaData* metaData = m_metaTable->GetMetaData();
		wxASSERT(metaData);
		const IMetaValueTypeCtor* clsFactory =
			metaData->GetTypeCtor(m_metaTable, eCtorMetaType::eCtorMetaType_TabularSection);
		wxASSERT(clsFactory);
		return clsFactory->GetClassName();
	}
	return _("<deleted metaobject>");
}

bool IValueTabularSectionDataObject::SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (m_readOnly || m_metaTable->IsNumberLine(id))
		return false;

	if (!appData->DesignerMode()) {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node != nullptr) {
			const IValueMetaObjectAttribute* attribute = m_metaTable->FindAnyAttributeObjectByFilter(id);
			wxASSERT(attribute);
			if (attribute == nullptr) return false;
			return node->SetValue(
				id, attribute->AdjustValue(varMetaVal), true
			);
		}
	}

	return false;
}

bool IValueTabularSectionDataObject::GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	if (m_metaTable->IsNumberLine(id)) {
		pvarMetaVal = GetRow(item) + 1;
		return true;
	}

	if (appData->DesignerMode()) {
		const IValueMetaObjectAttribute* attribute = m_metaTable->FindAnyAttributeObjectByFilter(id);
		wxASSERT(attribute);
		pvarMetaVal = attribute->CreateValue();
		return true;
	}

	wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
	if (node != nullptr)
		return node->GetValue(id, pvarMetaVal);

	return false;
}

bool IValueTabularSectionDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	const long lMethodAlias = m_methodHelper->GetMethodAlias(lMethodNum);
	if (lMethodAlias != eTabularSection)
		return false;

	const long lMethodData = m_methodHelper->GetMethodData(lMethodNum);
	switch (lMethodData)
	{
	case enAddValue:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectReturnLine>(this, GetItem(AppendRow()));
		return true;
	case enFind: {
		const wxDataViewItem& item = FindRowValue(*paParams[0], paParams[1]->GetString());
		if (item.IsOk())
			pvarRetValue = GetRowAt(item);
		return true;
	}
	case enCount:
		pvarRetValue = (unsigned int)GetRowCount();
		return true;
	case enDelete: {
		CValueTabularSectionDataObjectReturnLine* retLine = nullptr;
		if (paParams[0]->ConvertToValue(retLine)) {
			wxValueTableRow* node = GetViewData<wxValueTableRow>(retLine->GetLineItem());
			if (node != nullptr)
				IValueTable::Remove(node);
		}
		else {
			const number_t& number = paParams[0]->GetNumber();
			wxValueTableRow* node = GetViewData<wxValueTableRow>(GetItem(number.ToInt()));
			if (node != nullptr)
				IValueTable::Remove(node);
		}
		return true;
	}
	case enClear:
		Clear();
		return true;
	case enLoad:
		IValueTabularSectionDataObject::LoadDataFromTable(paParams[0]->ConvertToType<IValueTable>());
		return true;
	case enUnload:
		pvarRetValue = SaveDataToTable();
		return true;
	case enGetMetadata:
		pvarRetValue = m_metaTable;
		return true;
	}

	return false;
}

void IValueTabularSectionDataObject::RefreshTabularSection() {

	if (!CBackendException::IsEvalMode()) {

		CValue filterValue;
		for (long row = 0; row < IValueTable::GetRowCount(); row++) {

			const wxDataViewItem& item = IValueTable::GetItem(row); bool success_compare = true;
			for (auto filter : m_filterRow.m_filters) {

				if (filter.m_filterUse && GetValueByMetaID(item, filter.m_filterModel, filterValue)) {

					if (filter.m_filterComparison == eComparisonType_Equal)
						success_compare = success_compare && filter.m_filterValue == filterValue;
					else if (filter.m_filterComparison == eComparisonType_NotEqual)
						success_compare = success_compare && filter.m_filterValue != filterValue;

					if (!success_compare) break;
				}
			}

			IValueTable::Show(item, success_compare);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//               CValueTabularSectionDataObject                          //
//////////////////////////////////////////////////////////////////////

CValueTabularSectionDataObject::CValueTabularSectionDataObject() {}

CValueTabularSectionDataObject::CValueTabularSectionDataObject(IValueRecordDataObject* recordObject, CValueMetaObjectTableData* tableObject) :
	IValueTabularSectionDataObject(recordObject, tableObject)
{
}

//////////////////////////////////////////////////////////////////////
//               CValueTabularSectionDataObjectRef                       //
//////////////////////////////////////////////////////////////////////

CValueTabularSectionDataObjectRef::CValueTabularSectionDataObjectRef() : m_readAfter(false) {}

CValueTabularSectionDataObjectRef::CValueTabularSectionDataObjectRef(CValueReferenceDataObject* reference, CValueMetaObjectTableData* tableObject, bool readAfter) :
	IValueTabularSectionDataObject(reference, tableObject, true), m_readAfter(readAfter)
{
}

CValueTabularSectionDataObjectRef::CValueTabularSectionDataObjectRef(IValueRecordDataObjectRef* recordObject, CValueMetaObjectTableData* tableObject) :
	IValueTabularSectionDataObject(recordObject, tableObject), m_readAfter(false)
{
}

CValueTabularSectionDataObjectRef::CValueTabularSectionDataObjectRef(CValueSelectorRecordDataObject* selectorObject, CValueMetaObjectTableData* tableObject) :
	IValueTabularSectionDataObject((IValueDataObject*)selectorObject, tableObject), m_readAfter(false)
{
}

#include "backend/system/value/valueTable.h"

bool IValueTabularSectionDataObject::LoadDataFromTable(IValueTable* srcTable)
{
	if (m_readOnly)
		return false;

	IValueModelColumnCollection* colData = srcTable ?
		srcTable->GetColumnCollection() : nullptr;

	if (colData == nullptr)
		return false;

	wxArrayString columnName;
	for (unsigned int idx = 0; idx < colData->GetColumnCount(); idx++) {
		IValueModelColumnCollection::IValueModelColumnInfo* colInfo = colData->GetColumnInfo(idx);
		wxASSERT(colInfo);
		if (m_recordColumnCollection->GetColumnByName(colInfo->GetColumnName()) != nullptr) {
			columnName.push_back(colInfo->GetColumnName());
		}
	}

	unsigned int rowCount = srcTable->GetRowCount();
	for (unsigned int row = 0; row < rowCount; row++) {
		const wxDataViewItem& srcItem = srcTable->GetItem(row);
		const wxDataViewItem& dstItem = GetItem(AppendRow());
		for (auto colName : columnName) {
			CValue cRetValue;
			if (srcTable->GetValueByMetaID(srcItem, srcTable->GetColumnIDByName(colName), cRetValue)) {
				const meta_identifier_t& id = GetColumnIDByName(colName);
				if (id != wxNOT_FOUND) SetValueByMetaID(dstItem, id, cRetValue);
			}
		}
	}

	return true;
}

IValueTable* IValueTabularSectionDataObject::SaveDataToTable() const
{
	CValueTableMemory* valueTable = CValue::CreateAndPrepareValueRef<CValueTableMemory>();
	IValueModelColumnCollection* colData = valueTable->GetColumnCollection();
	for (unsigned int idx = 0; idx < m_recordColumnCollection->GetColumnCount() - 1; idx++) {
		IValueModelColumnCollection::IValueModelColumnInfo* colInfo = m_recordColumnCollection->GetColumnInfo(idx);
		wxASSERT(colInfo);
		IValueModelColumnCollection::IValueModelColumnInfo* newColInfo = colData->AddColumn(
			colInfo->GetColumnName(), colInfo->GetColumnType(), colInfo->GetColumnCaption(), colInfo->GetColumnWidth()
		);
		newColInfo->SetColumnID(colInfo->GetColumnID());
	}
	valueTable->PrepareNames();
	for (long row = 0; row < GetRowCount(); row++) {
		const wxDataViewItem& srcItem = GetItem(row);
		const wxDataViewItem& dstItem = valueTable->GetItem(valueTable->AppendRow());
		for (unsigned int col = 0; col < colData->GetColumnCount(); col++) {
			IValueModelColumnCollection::IValueModelColumnInfo* colInfo = colData->GetColumnInfo(col);
			wxASSERT(colInfo);
			if (m_metaTable->IsNumberLine(colInfo->GetColumnID()))
				continue;
			CValue cRetValue;
			if (GetValueByMetaID(srcItem, colInfo->GetColumnID(), cRetValue)) {
				const meta_identifier_t& id = GetColumnIDByName(colInfo->GetColumnName());
				if (id != wxNOT_FOUND) valueTable->SetValueByMetaID(dstItem, id, cRetValue);
			}
		}
	}

	return valueTable;
}

bool CValueTabularSectionDataObjectRef::SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal)
{
	if (varMetaVal != IValueTabularSectionDataObject::GetValueByMetaID(item, id)) {
		IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(
			m_objectValue->GetGuid()
		);
		bool result = IValueTabularSectionDataObject::SetValueByMetaID(item, id, varMetaVal);
		if (result && foundedForm != nullptr)
			foundedForm->Modify(true);
		return result;
	}

	return false;
}

bool CValueTabularSectionDataObjectRef::GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	return IValueTabularSectionDataObject::GetValueByMetaID(item, id, pvarMetaVal);
}

//////////////////////////////////////////////////////////////////////
//               CValueTabularSectionDataObjectReturnLine                //
//////////////////////////////////////////////////////////////////////

IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::CValueTabularSectionDataObjectReturnLine(IValueTabularSectionDataObject* ownerTable, const wxDataViewItem& line)
	: IValueModelReturnLine(line), m_ownerTable(ownerTable), m_methodHelper(new CMethodHelper()) {
}

IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::~CValueTabularSectionDataObjectReturnLine() {
	wxDELETE(m_methodHelper);
}

void IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	//set object name 
	wxString objectName;

	for (const auto object : m_ownerTable->m_metaTable->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (!object->GetObjectNameAsString(objectName))
			continue;
		m_methodHelper->AppendProp(
			objectName,
			true,
			!m_ownerTable->m_metaTable->IsNumberLine(object->GetMetaID()),
			object->GetMetaID()
		);
	}
}

bool IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return SetValueByMetaID(m_methodHelper->GetPropData(lPropNum), varPropVal);
}

bool IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	return GetValueByMetaID(m_methodHelper->GetPropData(lPropNum), pvarPropVal);
}

class_identifier_t IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::GetClassType() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_TabularSection_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::GetClassName() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_TabularSection_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString IValueTabularSectionDataObject::CValueTabularSectionDataObjectReturnLine::GetString() const
{
	const IValueMetaObject* metaTable = m_ownerTable->GetMetaObject();
	const IMetaData* metaData = metaTable->GetMetaData();
	wxASSERT(metaData);
	const IMetaValueTypeCtor* clsFactory =
		metaData->GetTypeCtor(metaTable, eCtorMetaType::eCtorMetaType_TabularSection_String);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//////////////////////////////////////////////////////////////////////
//               CValueTabularSectionDataObjectColumnCollection          //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection, IValueTable::IValueModelColumnCollection);

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionDataObjectColumnCollection() :
	IValueModelColumnCollection(),
	m_ownerTable(nullptr),
	m_methodHelper(nullptr)
{
}

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionDataObjectColumnCollection(IValueTabularSectionDataObject* ownerTable) :
	IValueModelColumnCollection(),
	m_ownerTable(ownerTable),
	m_methodHelper(new CMethodHelper())
{
	CValueMetaObjectTableData* metaTable = m_ownerTable->GetMetaObject();
	wxASSERT(metaTable);
	for (const auto object : metaTable->GetGenericAttributeArrayObject()) {
		if (metaTable->IsNumberLine(object->GetMetaID()))
			continue;
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			CValue::CreateAndPrepareValueRef<CValueTabularSectionColumnInfo>(object)
		);
	}
}

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::~CValueTabularSectionDataObjectColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendCoreException::Error(_("Index goes beyond array"));
		return false;
	}
	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;
	return true;
}

//////////////////////////////////////////////////////////////////////
//               CValueTabularSectionColumnInfo                     //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionColumnInfo::CValueTabularSectionColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionColumnInfo::CValueTabularSectionColumnInfo(IValueMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionColumnInfo::~CValueTabularSectionColumnInfo()
{
}

long IValueTabularSectionDataObject::AppendRow(unsigned int before)
{
	wxValueTableRow* rowData = new wxValueTableRow();
	for (const auto object : m_metaTable->GetGenericAttributeArrayObject()) {
		if (!m_metaTable->IsNumberLine(object->GetMetaID()))
			rowData->AppendTableValue(object->GetMetaID(), object->CreateValue());
	}

	if (before > 0)
		return IValueTable::Insert(rowData, before, !CBackendException::IsEvalMode());

	return IValueTable::Append(rowData, !CBackendException::IsEvalMode());
}

long CValueTabularSectionDataObjectRef::AppendRow(unsigned int before)
{
	if (!CBackendException::IsEvalMode())
		m_objectValue->Modify(true);
	return IValueTabularSectionDataObject::AppendRow(before);
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void IValueTabularSectionDataObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	if (m_readOnly) {
		m_methodHelper->AppendFunc("count", "count()", enCount, eTabularSection);
		m_methodHelper->AppendFunc("find", 2, "find(value, col)", enFind, eTabularSection);
		m_methodHelper->AppendFunc("unload", "unload()", enUnload, eTabularSection);
		m_methodHelper->AppendFunc("getMetadata", "getMetadata()", enGetMetadata, eTabularSection);
	}
	else {
		m_methodHelper->AppendFunc("add", "add()", enAddValue, eTabularSection);
		m_methodHelper->AppendFunc("count", "count()", enCount, eTabularSection);
		m_methodHelper->AppendFunc("find", 2, "find(value, col)", enFind, eTabularSection);
		m_methodHelper->AppendFunc("delete", 1, "delete(row)", enDelete, eTabularSection);
		m_methodHelper->AppendFunc("clear", "clear()", enClear, eTabularSection);
		m_methodHelper->AppendFunc("load", 1, "load(table)", enLoad, eTabularSection);
		m_methodHelper->AppendFunc("unload", "unload()", enUnload, eTabularSection);
		m_methodHelper->AppendFunc("getMetadata", "getMetadata()", enGetMetadata, eTabularSection);
	}
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection, "tabularSectionColumn", string_to_clsid("VL_TSCL"));
SYSTEM_TYPE_REGISTER(IValueTabularSectionDataObject::CValueTabularSectionDataObjectColumnCollection::CValueTabularSectionColumnInfo, "tabularSectionColumnInfo", string_to_clsid("VL_CI"));