////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list data 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"
#include "backend/srcExplorer.h"
#include "backend/appData.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueListDataObject, IValueTable);

wxIMPLEMENT_DYNAMIC_CLASS(CValueListDataObjectEnumRef, IValueListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(CValueListDataObjectRef, IValueListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(CValueListRegisterObject, IValueListDataObject);

wxIMPLEMENT_ABSTRACT_CLASS(IValueTreeDataObject, IValueTree);
wxIMPLEMENT_DYNAMIC_CLASS(CValueTreeDataObjectFolderRef, IValueTreeDataObject);

IValueListDataObject::IValueListDataObject(IValueMetaObjectGenericData* metaObject, const form_identifier_t& formType, bool choiceMode) :
	ISourceDataObject(),
	m_recordColumnCollection(new CValueDataObjectListColumnCollection(this, metaObject)),
	m_objGuid(choiceMode ? CGuid::newGuid() : metaObject->GetGuid()), m_methodHelper(new CMethodHelper())
{
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_filterRow.AppendFilter(
			object->GetMetaID(),
			object->GetName(),
			object->GetSynonym(),
			eComparisonType_Equal,
			object->GetTypeDesc(),
			object->CreateValue(),
			false
		);
	}
}

IValueListDataObject::~IValueListDataObject()
{
	wxDELETE(m_methodHelper);
}

///////////////////////////////////////////////////////////////////////////////

IValueTreeDataObject::IValueTreeDataObject(IValueMetaObjectGenericData* metaObject, const form_identifier_t& formType, bool choiceMode) :
	ISourceDataObject(),
	m_recordColumnCollection(new CValueDataObjectTreeColumnCollection(this, metaObject)),
	m_objGuid(choiceMode ? CGuid::newGuid() : metaObject->GetGuid()), m_methodHelper(new CMethodHelper())
{
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_filterRow.AppendFilter(
			object->GetMetaID(),
			object->GetName(),
			object->GetSynonym(),
			object->GetTypeDesc(),
			object->CreateValue()
		);
	}
}

IValueTreeDataObject::~IValueTreeDataObject()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//					  CValueDataObjectListColumnCollection               //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueListDataObject::CValueDataObjectListColumnCollection, IValueTable::IValueModelColumnCollection);

IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnCollection() :
	IValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnCollection(IValueListDataObject* ownerTable, IValueMetaObjectGenericData* metaObject) :
	IValueModelColumnCollection(), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(), new CValueDataObjectListColumnInfo(object));
	}
}

IValueListDataObject::CValueDataObjectListColumnCollection::~CValueDataObjectListColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IValueListDataObject::CValueDataObjectListColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IValueListDataObject::CValueDataObjectListColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();

	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendCoreException::Error("Index goes beyond array");
		return false;
	}

	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;

	return true;
}

wxIMPLEMENT_DYNAMIC_CLASS(IValueTreeDataObject::CValueDataObjectTreeColumnCollection, IValueTree::IValueModelColumnCollection);

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnCollection() :
	IValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnCollection(IValueTreeDataObject* ownerTable, IValueMetaObjectGenericData* metaObject) :
	IValueModelColumnCollection(), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			new CValueDataObjectTreeColumnInfo(object));
	}
}

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::~CValueDataObjectTreeColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IValueTreeDataObject::CValueDataObjectTreeColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IValueTreeDataObject::CValueDataObjectTreeColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		CBackendCoreException::Error("Index goes beyond array");
		return false;
	}
	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;
	return true;
}


//////////////////////////////////////////////////////////////////////
//							 CValueDataObjectListColumnInfo              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnInfo::CValueDataObjectListColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnInfo::CValueDataObjectListColumnInfo(IValueMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnInfo::~CValueDataObjectListColumnInfo()
{
}

wxIMPLEMENT_DYNAMIC_CLASS(IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnInfo, IValueTree::IValueModelColumnCollection::IValueModelColumnInfo);

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnInfo::CValueDataObjectTreeColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnInfo::CValueDataObjectTreeColumnInfo(IValueMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnInfo::~CValueDataObjectTreeColumnInfo()
{
}

//////////////////////////////////////////////////////////////////////
//					  CValueDataObjectListReturnLine                     //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IValueListDataObject::CValueDataObjectListReturnLine, IValueTable::IValueModelReturnLine);

IValueListDataObject::CValueDataObjectListReturnLine::CValueDataObjectListReturnLine(IValueListDataObject* ownerTable, const wxDataViewItem& line) :
	IValueModelReturnLine(line), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
}

IValueListDataObject::CValueDataObjectListReturnLine::~CValueDataObjectListReturnLine()
{
	wxDELETE(m_methodHelper);
}

void IValueListDataObject::CValueDataObjectListReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool IValueListDataObject::CValueDataObjectListReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IValueListDataObject::CValueDataObjectListReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	wxValueTableRow* node = m_ownerTable->GetViewData<wxValueTableRow>(m_lineItem);
	if (node == nullptr)
		return false;
	return node->GetValue(id, pvarPropVal);
}

wxIMPLEMENT_DYNAMIC_CLASS(IValueTreeDataObject::CValueDataObjectTreeReturnLine, IValueTree::IValueModelReturnLine);

IValueTreeDataObject::CValueDataObjectTreeReturnLine::CValueDataObjectTreeReturnLine(IValueTreeDataObject* ownerTable, const wxDataViewItem& line) :
	IValueModelReturnLine(line),
	m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
}

IValueTreeDataObject::CValueDataObjectTreeReturnLine::~CValueDataObjectTreeReturnLine()
{
	wxDELETE(m_methodHelper);
}

void IValueTreeDataObject::CValueDataObjectTreeReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool IValueTreeDataObject::CValueDataObjectTreeReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IValueTreeDataObject::CValueDataObjectTreeReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	wxValueTreeNode* node = m_ownerTable->GetViewData<wxValueTreeNode>(m_lineItem);
	if (node != nullptr) {
		return node->GetValue(id, pvarPropVal);
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CValueListDataObjectEnumRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		for (long row = 0; row < GetRowCount(); row++) {
			wxDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetGuid())
					return item;
			}
		}
	}
	return wxDataViewItem(nullptr);
}

wxDataViewItem CValueListDataObjectEnumRef::FindRowValue(IValueModelReturnLine* retLine) const
{
	wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node](wxValueTableRow* row)
		{
			return node->GetGuid() == ((wxValueTableEnumRow*)row)->GetGuid();
		}
	);
	if (it != m_nodeValues.end()) return wxDataViewItem(*it);
	return wxDataViewItem(nullptr);
}

CValueListDataObjectEnumRef::CValueListDataObjectEnumRef(IValueMetaObjectRecordDataEnumRef* metaObject, const form_identifier_t& formType, bool choiceMode) :
	IValueListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
	IValueListDataObject::AppendSort(m_metaObject->GetDataOrder(), true, true, true);
	IValueListDataObject::AppendSort(m_metaObject->GetDataReference(), true, true, true);
}

CSourceExplorer CValueListDataObjectEnumRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);
	srcHelper.AppendSource(m_metaObject->GetDataReference(), true, true);
	return srcHelper;
}

bool CValueListDataObjectEnumRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CValueListDataObjectEnumRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(GetSelection());
	if (node == nullptr)
		return;
	wxASSERT(srcForm);
	CValueReferenceDataObject* dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
	if (dataValueRef != nullptr) {
		srcForm->NotifyChoice(dataValueRef->GetValue());
	}
}

#include "backend/objCtor.h"

class_identifier_t CValueListDataObjectEnumRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueListDataObjectEnumRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueListDataObjectEnumRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CValueListDataObjectRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		for (long row = 0; row < GetRowCount(); row++) {
			wxDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableListRow* node = GetViewData<wxValueTableListRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetGuid())
					return item;
			}
		}
	}
	return wxDataViewItem(nullptr);
}

wxDataViewItem CValueListDataObjectRef::FindRowValue(IValueModelReturnLine* retLine) const
{
	wxValueTableListRow* node = GetViewData<wxValueTableListRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node](wxValueTableRow* row)
		{
			return node->GetGuid() == ((wxValueTableListRow*)row)->GetGuid();
		}
	);
	if (it != m_nodeValues.end()) return wxDataViewItem(*it);
	return wxDataViewItem(nullptr);
}

CValueListDataObjectRef::CValueListDataObjectRef(IValueMetaObjectRecordDataMutableRef* metaObject, const form_identifier_t& formType, bool choiceMode) :
	IValueListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
}

CSourceExplorer CValueListDataObjectRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (m_metaObject->IsDataReference(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataDeletionMark(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else
			srcHelper.AppendSource(object, true, true);
	}

	return srcHelper;
}

bool CValueListDataObjectRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CValueListDataObjectRef::AddValue(unsigned int before)
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		try {
			CValuePtr<IValueRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CValueListDataObjectRef::CopyValue()
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IValueRecordDataObjectRef> dataValueObject(metaObject->CopyObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CValueListDataObjectRef::EditValue()
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			IValueRecordDataObjectRef* dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CValueListDataObjectRef::DeleteValue()
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IValueRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr)
				dataValueObject->DeleteObject();
			IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr)
				valueListForm->UpdateForm();
		}
		catch (...)
		{
		}
	}
}

void CValueListDataObjectRef::MarkAsDeleteValue()
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IValueRecordDataObjectRef> dataValueObject = metaObject->CreateObjectValue(node->GetGuid());
			if (dataValueObject != nullptr)
				dataValueObject->SetDeletionMark(true);
			IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr)
				valueListForm->UpdateForm();
		}
		catch (...)
		{
		}
	}
}

void CValueListDataObjectRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	try {
		CValuePtr<CValueReferenceDataObject> dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
		if (dataValueRef != nullptr)
			srcForm->NotifyChoice(dataValueRef->GetValue());
	}
	catch (...)
	{
	}
}

class_identifier_t CValueListDataObjectRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueListDataObjectRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueListDataObjectRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CValueTreeDataObjectFolderRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const CGuid&)> findGuid = [&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const CGuid& guid)
			{
				if (guid == parent->GetGuid()) {
					foundedNode = parent; return;
				}
				else if (foundedNode != nullptr) {
					return;
				}
				for (unsigned int n = 0; n < parent->GetChildCount(); n++) {
					wxValueTreeListNode* node = dynamic_cast<wxValueTreeListNode*>(parent->GetChild(n));
					if (node != nullptr)
						findGuid(node, foundedNode, guid);
					if (foundedNode != nullptr)
						break;
				}
			};
		wxValueTreeListNode* foundedNode = nullptr;
		for (unsigned int child = 0; child < GetRoot()->GetChildCount(); child++) {
			wxValueTreeListNode* node = dynamic_cast<wxValueTreeListNode*>(GetRoot()->GetChild(child));
			if (node != nullptr)
				findGuid(node, foundedNode, pRefData->GetGuid());
			if (foundedNode != nullptr)
				break;
		}
		if (foundedNode != nullptr)
			return wxDataViewItem(foundedNode);
	}
	return wxDataViewItem(nullptr);
}

wxDataViewItem CValueTreeDataObjectFolderRef::FindRowValue(IValueModelReturnLine* retLine) const
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(retLine->GetLineItem());
	std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const CGuid&)> findGuid =
		[&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const CGuid& guid)
		{
			if (guid == parent->GetGuid()) { foundedNode = parent; return; }
			else if (foundedNode != nullptr) { return; }

			for (unsigned int n = 0; n < parent->GetChildCount(); n++) {
				wxValueTreeListNode* child = dynamic_cast<wxValueTreeListNode*>(parent->GetChild(n));
				if (child != nullptr)
					findGuid(child, foundedNode, guid);
				if (foundedNode != nullptr) break;
			}
		};
	wxValueTreeListNode* foundedNode = nullptr;
	for (unsigned int c = 0; c < GetRoot()->GetChildCount(); c++) {
		wxValueTreeListNode* child = dynamic_cast<wxValueTreeListNode*>(GetRoot()->GetChild(c));
		if (child != nullptr) findGuid(child, foundedNode, node->GetGuid());
		if (foundedNode != nullptr) break;
	}
	if (foundedNode != nullptr) return wxDataViewItem(foundedNode);
	return wxDataViewItem(nullptr);
}

CValueTreeDataObjectFolderRef::CValueTreeDataObjectFolderRef(IValueMetaObjectRecordDataHierarchyMutableRef* metaObject, const form_identifier_t& formType,
	int listMode, bool choiceMode) : IValueTreeDataObject(metaObject, formType, choiceMode),
	m_metaObject(metaObject), m_listMode(listMode), m_choiceMode(choiceMode)
{
	IValueTreeDataObject::AppendSort(m_metaObject->GetDataIsFolder(), false, true, true);
	IValueTreeDataObject::AppendSort(m_metaObject->GetDataCode(), true, false);
	IValueTreeDataObject::AppendSort(m_metaObject->GetDataDescription(), true);
	IValueTreeDataObject::AppendSort(m_metaObject->GetDataReference());
}

CSourceExplorer CValueTreeDataObjectFolderRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		if (m_metaObject->IsDataReference(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataDeletionMark(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataPredefinedName(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataParent(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataFolder(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else
			srcHelper.AppendSource(object, true, true);
	}

	return srcHelper;
}

bool CValueTreeDataObjectFolderRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CValueTreeDataObjectFolderRef::AddValue(unsigned int before)
{
	CValue isFolder = true; CValue cParent;
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node != nullptr) {
		node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);
		if (!isFolder.GetBoolean())
			node->GetValue(*m_metaObject->GetDataParent(), cParent);
		else
			node->GetValue(*m_metaObject->GetDataReference(), cParent);
	}

	CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(eObjectMode::OBJECT_ITEM));

	if (dataValueFolderObject != nullptr) {
		dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
		dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
}

void CValueTreeDataObjectFolderRef::AddFolderValue(unsigned int before)
{
	CValue isFolder = true; CValue cParent;
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node != nullptr) {
		node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);
		if (!isFolder.GetBoolean())
			node->GetValue(*m_metaObject->GetDataParent(), cParent);
		else
			node->GetValue(*m_metaObject->GetDataReference(), cParent);
	}

	try {
		CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(eObjectMode::OBJECT_FOLDER));
		if (dataValueFolderObject != nullptr) {
			dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
	}
	catch (...)
	{

	}
}

void CValueTreeDataObjectFolderRef::CopyValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject = m_metaObject->CopyObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid());
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (...)
	{
	}
}

void CValueTreeDataObjectFolderRef::EditValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr) dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (...)
	{
	}
}

void CValueTreeDataObjectFolderRef::DeleteValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->DeleteObject();
		IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
		if (valueListForm != nullptr) valueListForm->UpdateForm();
	}
	catch (...)
	{
	}

}

void CValueTreeDataObjectFolderRef::MarkAsDeleteValue()
{
	IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
		if (node == nullptr)
			return;

		CValue isFolder = false;
		node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

		try {
			CValuePtr<IValueRecordDataObjectFolderRef> dataValueFolderObject(metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
			if (dataValueFolderObject != nullptr)
				dataValueFolderObject->SetDeletionMark(true);
			IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr) valueListForm->UpdateForm();
		}
		catch (...)
		{
		}
	}
}

void CValueTreeDataObjectFolderRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	CValue cIsFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), cIsFolder);

	try {
		CValuePtr<CValueReferenceDataObject> dataValueFolderRef(m_metaObject->FindObjectValue(node->GetGuid()));
		if (m_listMode == LIST_FOLDER && cIsFolder.GetBoolean())
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
		else if (m_listMode == LIST_ITEM && !cIsFolder.GetBoolean())
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
		else if (m_listMode == LIST_ITEM_FOLDER)
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
	}
	catch (...)
	{
	}
}

#include "backend/objCtor.h"

class_identifier_t CValueTreeDataObjectFolderRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueTreeDataObjectFolderRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueTreeDataObjectFolderRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CValueListRegisterObject::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	IValueRecordManagerObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		IValueMetaObjectRegisterData* metaObject = GetMetaObject();
		wxASSERT(metaObject);
		for (long row = 0; row < GetRowCount(); row++) {
			wxDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetUniquePairKey(metaObject))
					return item;
			}
		}
	}

	return wxDataViewItem(nullptr);
}

wxDataViewItem CValueListRegisterObject::FindRowValue(IValueModelReturnLine* retLine) const
{
	IValueMetaObjectRegisterData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node, metaObject](wxValueTableRow* row)
		{
			return node->GetUniquePairKey(metaObject) == ((wxValueTableKeyRow*)row)->GetUniquePairKey(metaObject);
		}
	);
	if (it != m_nodeValues.end()) return wxDataViewItem(*it);
	return wxDataViewItem(nullptr);
}

CValueListRegisterObject::CValueListRegisterObject(IValueMetaObjectRegisterData* metaObject, const form_identifier_t& formType) :
	IValueListDataObject(metaObject, formType), m_metaObject(metaObject)
{
	if (m_metaObject->HasRecorder()) {
		if (m_metaObject->HasPeriod()) IValueListDataObject::AppendSort(metaObject->GetRegisterPeriod());
		IValueListDataObject::AppendSort(metaObject->GetRegisterRecorder());
		IValueListDataObject::AppendSort(metaObject->GetRegisterLineNumber());
	}
	else if (m_metaObject->HasPeriod()) {
		IValueListDataObject::AppendSort(metaObject->GetRegisterPeriod());
	}

	for (auto& dimension : m_metaObject->GetGenericDimentionArrayObject()) {
		IValueListDataObject::AppendSort(dimension, true, true, true);
	}
}

CSourceExplorer CValueListRegisterObject::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);

	for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
		srcHelper.AppendSource(object);
	}

	return srcHelper;
}

bool CValueListRegisterObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CValueListRegisterObject::AddValue(unsigned int before)
{
	if (m_metaObject != nullptr && m_metaObject->HasRecordManager()) {
		try {
			CValuePtr<IValueRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CValueListRegisterObject::CopyValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IValueRecordManagerObject> dataValueObject(m_metaObject->CopyRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (...)
			{
			}
		}
	}
}

void CValueListRegisterObject::EditValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IValueRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (...)
			{
			}
		}
		else {
			try {
				const CValueMetaObjectAttributePredefined* metaRecorder = m_metaObject->GetRegisterRecorder();
				if (metaRecorder != nullptr) {
					CValue recorderVal = node->GetTableValue(metaRecorder->GetMetaID());
					recorderVal.ShowValue();
				}
			}
			catch (...)
			{
			}
		}
	}
}

void CValueListRegisterObject::DeleteValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IValueRecordManagerObject> dataValueObject = m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject));
				if (dataValueObject != nullptr)
					dataValueObject->DeleteRegister();
				IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
				if (valueListForm != nullptr) valueListForm->UpdateForm();
			}
			catch (...)
			{
			}
		}
	}
}

class_identifier_t CValueListRegisterObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueListRegisterObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CValueListRegisterObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CValueListDataObjectEnumRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool CValueListDataObjectEnumRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRefresh:
		CallRefreshModel();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void CValueListDataObjectRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool CValueListDataObjectRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRefresh:
		CallRefreshModel();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void CValueTreeDataObjectFolderRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool CValueTreeDataObjectFolderRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRefresh:
		CallRefreshModel();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void CValueListRegisterObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool CValueListRegisterObject::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enRefresh:
		CallRefreshModel();
		return true;
	}
	return false;
}

//****************************************************************************
//*                              Override object                          *
//****************************************************************************

enum
{
	enChoiceMode
};

bool CValueListDataObjectEnumRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CValueListDataObjectEnumRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CValueListDataObjectRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CValueListDataObjectRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CValueTreeDataObjectFolderRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CValueTreeDataObjectFolderRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CValueListRegisterObject::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CValueListRegisterObject::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(IValueListDataObject::CValueDataObjectListColumnCollection, "ListColumn", string_to_clsid("VL_LVC"));
SYSTEM_TYPE_REGISTER(IValueListDataObject::CValueDataObjectListColumnCollection::CValueDataObjectListColumnInfo, "ListColumnInfo", string_to_clsid("VL_LCI"));
SYSTEM_TYPE_REGISTER(IValueListDataObject::CValueDataObjectListReturnLine, "ListValueRow", string_to_clsid("VL_LVR"));

SYSTEM_TYPE_REGISTER(IValueTreeDataObject::CValueDataObjectTreeColumnCollection, "TreeColumn", string_to_clsid("VL_TVC"));
SYSTEM_TYPE_REGISTER(IValueTreeDataObject::CValueDataObjectTreeColumnCollection::CValueDataObjectTreeColumnInfo, "TreeColumnInfo", string_to_clsid("VL_TCI"));
SYSTEM_TYPE_REGISTER(IValueTreeDataObject::CValueDataObjectTreeReturnLine, "TreeValueRow", string_to_clsid("VL_TVR"));