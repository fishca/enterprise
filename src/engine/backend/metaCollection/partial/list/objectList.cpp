////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list data 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"
#include "backend/srcExplorer.h"
#include "backend/appData.h"

wxIMPLEMENT_ABSTRACT_CLASS(IListDataObject, IValueTable);

wxIMPLEMENT_DYNAMIC_CLASS(CListDataObjectEnumRef, IListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(CListDataObjectRef, IListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(CListRegisterObject, IListDataObject);

wxIMPLEMENT_ABSTRACT_CLASS(ITreeDataObject, IValueTree);
wxIMPLEMENT_DYNAMIC_CLASS(CTreeDataObjectFolderRef, ITreeDataObject);

IListDataObject::IListDataObject(IMetaObjectGenericData* metaObject, const form_identifier_t& formType, bool choiceMode) :
	ISourceDataObject(),
	m_recordColumnCollection(new CDataObjectListColumnCollection(this, metaObject)),
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

IListDataObject::~IListDataObject()
{
	wxDELETE(m_methodHelper);
}

///////////////////////////////////////////////////////////////////////////////

ITreeDataObject::ITreeDataObject(IMetaObjectGenericData* metaObject, const form_identifier_t& formType, bool choiceMode) :
	ISourceDataObject(),
	m_recordColumnCollection(new CDataObjectTreeColumnCollection(this, metaObject)),
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

ITreeDataObject::~ITreeDataObject()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//					  CDataObjectListColumnCollection               //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IListDataObject::CDataObjectListColumnCollection, IValueTable::IValueModelColumnCollection);

IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnCollection() :
	IValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnCollection(IListDataObject* ownerTable, IMetaObjectGenericData* metaObject) :
	IValueModelColumnCollection(), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(), new CDataObjectListColumnInfo(object));
	}
}

IListDataObject::CDataObjectListColumnCollection::~CDataObjectListColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool IListDataObject::CDataObjectListColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool IListDataObject::CDataObjectListColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
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

wxIMPLEMENT_DYNAMIC_CLASS(ITreeDataObject::CDataObjectTreeColumnCollection, IValueTree::IValueModelColumnCollection);

ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnCollection() :
	IValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnCollection(ITreeDataObject* ownerTable, IMetaObjectGenericData* metaObject) :
	IValueModelColumnCollection(), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			new CDataObjectTreeColumnInfo(object));
	}
}

ITreeDataObject::CDataObjectTreeColumnCollection::~CDataObjectTreeColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool ITreeDataObject::CDataObjectTreeColumnCollection::SetAt(const CValue& varKeyValue, const CValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool ITreeDataObject::CDataObjectTreeColumnCollection::GetAt(const CValue& varKeyValue, CValue& pvarValue) //индекс массива должен начинаться с 0
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
//							 CDataObjectListColumnInfo              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnInfo, IValueTable::IValueModelColumnCollection::IValueModelColumnInfo);

IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnInfo::CDataObjectListColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnInfo::CDataObjectListColumnInfo(IMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnInfo::~CDataObjectListColumnInfo()
{
}

wxIMPLEMENT_DYNAMIC_CLASS(ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnInfo, IValueTree::IValueModelColumnCollection::IValueModelColumnInfo);

ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnInfo::CDataObjectTreeColumnInfo() :
	IValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnInfo::CDataObjectTreeColumnInfo(IMetaObjectAttribute* attribute) :
	IValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnInfo::~CDataObjectTreeColumnInfo()
{
}

//////////////////////////////////////////////////////////////////////
//					  CDataObjectListReturnLine                     //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(IListDataObject::CDataObjectListReturnLine, IValueTable::IValueModelReturnLine);

IListDataObject::CDataObjectListReturnLine::CDataObjectListReturnLine(IListDataObject* ownerTable, const wxDataViewItem& line) :
	IValueModelReturnLine(line), m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
}

IListDataObject::CDataObjectListReturnLine::~CDataObjectListReturnLine()
{
	wxDELETE(m_methodHelper);
}

void IListDataObject::CDataObjectListReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool IListDataObject::CDataObjectListReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool IListDataObject::CDataObjectListReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	wxValueTableRow* node = m_ownerTable->GetViewData<wxValueTableRow>(m_lineItem);
	if (node == nullptr)
		return false;
	return node->GetValue(id, pvarPropVal);
}

wxIMPLEMENT_DYNAMIC_CLASS(ITreeDataObject::CDataObjectTreeReturnLine, IValueTree::IValueModelReturnLine);

ITreeDataObject::CDataObjectTreeReturnLine::CDataObjectTreeReturnLine(ITreeDataObject* ownerTable, const wxDataViewItem& line) :
	IValueModelReturnLine(line),
	m_methodHelper(new CMethodHelper()), m_ownerTable(ownerTable)
{
}

ITreeDataObject::CDataObjectTreeReturnLine::~CDataObjectTreeReturnLine()
{
	wxDELETE(m_methodHelper);
}

void ITreeDataObject::CDataObjectTreeReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool ITreeDataObject::CDataObjectTreeReturnLine::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	return false;
}

bool ITreeDataObject::CDataObjectTreeReturnLine::GetPropVal(const long lPropNum, CValue& pvarPropVal)
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

wxDataViewItem CListDataObjectEnumRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CReferenceDataObject* pRefData = nullptr;
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

wxDataViewItem CListDataObjectEnumRef::FindRowValue(IValueModelReturnLine* retLine) const
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

CListDataObjectEnumRef::CListDataObjectEnumRef(IMetaObjectRecordDataEnumRef* metaObject, const form_identifier_t& formType, bool choiceMode) :
	IListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
	IListDataObject::AppendSort(m_metaObject->GetDataOrder(), true, true, true);
	IListDataObject::AppendSort(m_metaObject->GetDataReference(), true, true, true);
}

CSourceExplorer CListDataObjectEnumRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);
	srcHelper.AppendSource(m_metaObject->GetDataReference(), true, true);
	return srcHelper;
}

bool CListDataObjectEnumRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CListDataObjectEnumRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(GetSelection());
	if (node == nullptr)
		return;
	wxASSERT(srcForm);
	CReferenceDataObject* dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
	if (dataValueRef != nullptr) {
		srcForm->NotifyChoice(dataValueRef->GetValue());
	}
}

#include "backend/objCtor.h"

class_identifier_t CListDataObjectEnumRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CListDataObjectEnumRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CListDataObjectEnumRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CListDataObjectRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CReferenceDataObject* pRefData = nullptr;
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

wxDataViewItem CListDataObjectRef::FindRowValue(IValueModelReturnLine* retLine) const
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

CListDataObjectRef::CListDataObjectRef(IMetaObjectRecordDataMutableRef* metaObject, const form_identifier_t& formType, bool choiceMode) :
	IListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
}

CSourceExplorer CListDataObjectRef::GetSourceExplorer() const
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

bool CListDataObjectRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CListDataObjectRef::AddValue(unsigned int before)
{
	IMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		try {
			CValuePtr<IRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CListDataObjectRef::CopyValue()
{
	IMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IRecordDataObjectRef> dataValueObject(metaObject->CopyObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CListDataObjectRef::EditValue()
{
	IMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			IRecordDataObjectRef* dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CListDataObjectRef::DeleteValue()
{
	IMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
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

void CListDataObjectRef::MarkAsDeleteValue()
{
	IMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			CValuePtr<IRecordDataObjectRef> dataValueObject = metaObject->CreateObjectValue(node->GetGuid());
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

void CListDataObjectRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	try {
		CValuePtr<CReferenceDataObject> dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
		if (dataValueRef != nullptr)
			srcForm->NotifyChoice(dataValueRef->GetValue());
	}
	catch (...)
	{
	}
}

class_identifier_t CListDataObjectRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CListDataObjectRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CListDataObjectRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CTreeDataObjectFolderRef::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	CReferenceDataObject* pRefData = nullptr;
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

wxDataViewItem CTreeDataObjectFolderRef::FindRowValue(IValueModelReturnLine* retLine) const
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

CTreeDataObjectFolderRef::CTreeDataObjectFolderRef(IMetaObjectRecordDataHierarchyMutableRef* metaObject, const form_identifier_t& formType,
	int listMode, bool choiceMode) : ITreeDataObject(metaObject, formType, choiceMode),
	m_metaObject(metaObject), m_listMode(listMode), m_choiceMode(choiceMode)
{
	ITreeDataObject::AppendSort(m_metaObject->GetDataIsFolder(), false, true, true);
	ITreeDataObject::AppendSort(m_metaObject->GetDataCode(), true, false);
	ITreeDataObject::AppendSort(m_metaObject->GetDataDescription(), true);
	ITreeDataObject::AppendSort(m_metaObject->GetDataReference());
}

CSourceExplorer CTreeDataObjectFolderRef::GetSourceExplorer() const
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
		else if (m_metaObject->IsDataParent(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else if (m_metaObject->IsDataFolder(object->GetMetaID()))
			srcHelper.AppendSource(object, true, false);
		else
			srcHelper.AppendSource(object, true, true);
	}

	return srcHelper;
}

bool CTreeDataObjectFolderRef::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CTreeDataObjectFolderRef::AddValue(unsigned int before)
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

	CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(eObjectMode::OBJECT_ITEM));

	if (dataValueFolderObject != nullptr) {
		dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
		dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
}

void CTreeDataObjectFolderRef::AddFolderValue(unsigned int before)
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
		CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(eObjectMode::OBJECT_FOLDER));
		if (dataValueFolderObject != nullptr) {
			dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
	}
	catch (...)
	{

	}
}

void CTreeDataObjectFolderRef::CopyValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject = m_metaObject->CopyObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid());
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (...)
	{
	}
}

void CTreeDataObjectFolderRef::EditValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr) dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (...)
	{
	}
}

void CTreeDataObjectFolderRef::DeleteValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->DeleteObject();
		IBackendValueForm* valueListForm = IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
		if (valueListForm != nullptr) valueListForm->UpdateForm();
	}
	catch (...)
	{
	}

}

void CTreeDataObjectFolderRef::MarkAsDeleteValue()
{
	IMetaObjectRecordDataHierarchyMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
		if (node == nullptr)
			return;

		CValue isFolder = false;
		node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

		try {
			CValuePtr<IRecordDataObjectFolderRef> dataValueFolderObject(metaObject->CreateObjectValue(isFolder.GetBoolean() ? eObjectMode::OBJECT_FOLDER : eObjectMode::OBJECT_ITEM, node->GetGuid()));
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

void CTreeDataObjectFolderRef::ChooseValue(IBackendValueForm* srcForm)
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	CValue cIsFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), cIsFolder);

	try {
		CValuePtr<CReferenceDataObject> dataValueFolderRef(m_metaObject->FindObjectValue(node->GetGuid()));
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

class_identifier_t CTreeDataObjectFolderRef::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CTreeDataObjectFolderRef::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CTreeDataObjectFolderRef::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxDataViewItem CListRegisterObject::FindRowValue(const CValue& varValue, const wxString& colName) const
{
	IRecordManagerObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		IMetaObjectRegisterData* metaObject = GetMetaObject();
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

wxDataViewItem CListRegisterObject::FindRowValue(IValueModelReturnLine* retLine) const
{
	IMetaObjectRegisterData* metaObject = GetMetaObject();
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

CListRegisterObject::CListRegisterObject(IMetaObjectRegisterData* metaObject, const form_identifier_t& formType) :
	IListDataObject(metaObject, formType), m_metaObject(metaObject)
{
	if (m_metaObject->HasRecorder()) {
		if (m_metaObject->HasPeriod()) IListDataObject::AppendSort(metaObject->GetRegisterPeriod());
		IListDataObject::AppendSort(metaObject->GetRegisterRecorder());
		IListDataObject::AppendSort(metaObject->GetRegisterLineNumber());
	}
	else if (m_metaObject->HasPeriod()) {
		IListDataObject::AppendSort(metaObject->GetRegisterPeriod());
	}

	for (auto& dimension : m_metaObject->GetGenericDimentionArrayObject()) {
		IListDataObject::AppendSort(dimension, true, true, true);
	}
}

CSourceExplorer CListRegisterObject::GetSourceExplorer() const
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

bool CListRegisterObject::GetModel(IValueModel*& tableValue, const meta_identifier_t& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void CListRegisterObject::AddValue(unsigned int before)
{
	if (m_metaObject != nullptr && m_metaObject->HasRecordManager()) {
		try {
			CValuePtr<IRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (...)
		{
		}
	}
}

void CListRegisterObject::CopyValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IRecordManagerObject> dataValueObject(m_metaObject->CopyRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (...)
			{
			}
		}
	}
}

void CListRegisterObject::EditValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<IBackendControlFrame*>(IBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (...)
			{
			}
		}
		else {
			try {
				const CMetaObjectAttributePredefined* metaRecorder = m_metaObject->GetRegisterRecorder();
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

void CListRegisterObject::DeleteValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				CValuePtr<IRecordManagerObject> dataValueObject = m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject));
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

class_identifier_t CListRegisterObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CListRegisterObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString CListRegisterObject::GetString() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void CListDataObjectEnumRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("choiceMode"));
	m_methodHelper->AppendProc(wxT("refresh"), "refresh()");
}

bool CListDataObjectEnumRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
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

void CListDataObjectRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("choiceMode"));
	m_methodHelper->AppendProc(wxT("refresh"), "refresh()");
}

bool CListDataObjectRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
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

void CTreeDataObjectFolderRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("choiceMode"));
	m_methodHelper->AppendProc(wxT("refresh"), "refresh()");
}

bool CTreeDataObjectFolderRef::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
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

void CListRegisterObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProc(wxT("refresh"), "refresh()");
}

bool CListRegisterObject::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
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

bool CListDataObjectEnumRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CListDataObjectEnumRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CListDataObjectRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CListDataObjectRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CTreeDataObjectFolderRef::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CTreeDataObjectFolderRef::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool CListRegisterObject::SetPropVal(const long lPropNum, const CValue& value) //setting attribute
{
	return false;
}

bool CListRegisterObject::GetPropVal(const long lPropNum, CValue& pvarPropVal) //attribute value
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(IListDataObject::CDataObjectListColumnCollection, "listColumn", string_to_clsid("VL_LVC"));
SYSTEM_TYPE_REGISTER(IListDataObject::CDataObjectListColumnCollection::CDataObjectListColumnInfo, "listColumnInfo", string_to_clsid("VL_LCI"));
SYSTEM_TYPE_REGISTER(IListDataObject::CDataObjectListReturnLine, "listValueRow", string_to_clsid("VL_LVR"));

SYSTEM_TYPE_REGISTER(ITreeDataObject::CDataObjectTreeColumnCollection, "treeColumn", string_to_clsid("VL_TVC"));
SYSTEM_TYPE_REGISTER(ITreeDataObject::CDataObjectTreeColumnCollection::CDataObjectTreeColumnInfo, "treeColumnInfo", string_to_clsid("VL_TCI"));
SYSTEM_TYPE_REGISTER(ITreeDataObject::CDataObjectTreeReturnLine, "treeValueRow", string_to_clsid("VL_TVR"));