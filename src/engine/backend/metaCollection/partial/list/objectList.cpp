////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list data 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"
#include "backend/srcExplorer.h"
#include "backend/system/systemManager.h"

#include "backend/appData.h"

wxIMPLEMENT_ABSTRACT_CLASS(ibValueListDataObject, ibValueModelTable);

wxIMPLEMENT_DYNAMIC_CLASS(ibValueListDataObjectEnumRef, ibValueListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueListDataObjectRef, ibValueListDataObject);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueListRegisterObject, ibValueListDataObject);

wxIMPLEMENT_ABSTRACT_CLASS(ibValueModelTreeDataObject, ibValueModelTree);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTreeDataObjectFolderRef, ibValueModelTreeDataObject);

ibValueListDataObject::ibValueListDataObject(ibValueMetaObjectGenericData* metaObject, const ibFormID& formType, bool choiceMode) :
	ibSourceDataObject(),
	m_recordColumnCollection(new ibValueDataObjectListColumnCollection(this, metaObject)),
	m_objGuid(choiceMode ? ibGuid::newGuid() : metaObject->GetGuid()), m_methodHelper(new ibValueMethodHelper())
{
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_filterRow.AppendFilter(
			object->GetMetaID(),
			object->GetName(),
			object->GetSynonym(),
			ibComparisonType_Equal,
			object->GetTypeDesc(),
			object->CreateValue(),
			false
		);
	}
}

ibValueListDataObject::~ibValueListDataObject()
{
	wxDELETE(m_methodHelper);
}

///////////////////////////////////////////////////////////////////////////////

ibValueModelTreeDataObject::ibValueModelTreeDataObject(ibValueMetaObjectGenericData* metaObject, const ibFormID& formType, bool choiceMode) :
	ibSourceDataObject(),
	m_recordColumnCollection(new ibValueDataObjectTreeColumnCollection(this, metaObject)),
	m_objGuid(choiceMode ? ibGuid::newGuid() : metaObject->GetGuid()), m_methodHelper(new ibValueMethodHelper())
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

ibValueModelTreeDataObject::~ibValueModelTreeDataObject()
{
	wxDELETE(m_methodHelper);
}

//////////////////////////////////////////////////////////////////////
//					  ibValueDataObjectListColumnCollection               //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueListDataObject::ibValueDataObjectListColumnCollection, ibValueModelTable::ibValueModelColumnCollection);

ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnCollection() :
	ibValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnCollection(ibValueListDataObject* ownerTable, ibValueMetaObjectGenericData* metaObject) :
	ibValueModelColumnCollection(), m_methodHelper(new ibValueMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(), new ibValueDataObjectListColumnInfo(object));
	}
}

ibValueListDataObject::ibValueDataObjectListColumnCollection::~ibValueDataObjectListColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool ibValueListDataObject::ibValueDataObjectListColumnCollection::SetAt(const ibValue& varKeyValue, const ibValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool ibValueListDataObject::ibValueDataObjectListColumnCollection::GetAt(const ibValue& varKeyValue, ibValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();

	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		ibBackendCoreException::Error("Index goes beyond array");
		return false;
	}

	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;

	return true;
}

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection, ibValueModelTree::ibValueModelColumnCollection);

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnCollection() :
	ibValueModelColumnCollection(), m_methodHelper(nullptr), m_ownerTable(nullptr)
{
}

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnCollection(ibValueModelTreeDataObject* ownerTable, ibValueMetaObjectGenericData* metaObject) :
	ibValueModelColumnCollection(), m_methodHelper(new ibValueMethodHelper()), m_ownerTable(ownerTable)
{
	wxASSERT(metaObject);

	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_listColumnInfo.insert_or_assign(object->GetMetaID(),
			new ibValueDataObjectTreeColumnInfo(object));
	}
}

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::~ibValueDataObjectTreeColumnCollection()
{
	wxDELETE(m_methodHelper);
}

bool ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::SetAt(const ibValue& varKeyValue, const ibValue& varValue)//индекс массива должен начинаться с 0
{
	return false;
}

bool ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::GetAt(const ibValue& varKeyValue, ibValue& pvarValue) //индекс массива должен начинаться с 0
{
	unsigned int index = varKeyValue.GetUInteger();
	if ((index < 0 || index >= m_listColumnInfo.size() && !appData->DesignerMode())) {
		ibBackendCoreException::Error("Index goes beyond array");
		return false;
	}
	auto it = m_listColumnInfo.begin();
	std::advance(it, index);
	pvarValue = it->second;
	return true;
}


//////////////////////////////////////////////////////////////////////
//							 ibValueDataObjectListColumnInfo              //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnInfo, ibValueModelTable::ibValueModelColumnCollection::ibValueModelColumnInfo);

ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnInfo::ibValueDataObjectListColumnInfo() :
	ibValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnInfo::ibValueDataObjectListColumnInfo(ibValueMetaObjectAttributeBase* attribute) :
	ibValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnInfo::~ibValueDataObjectListColumnInfo()
{
}

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnInfo, ibValueModelTree::ibValueModelColumnCollection::ibValueModelColumnInfo);

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnInfo::ibValueDataObjectTreeColumnInfo() :
	ibValueModelColumnInfo(), m_metaAttribute(nullptr)
{
}

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnInfo::ibValueDataObjectTreeColumnInfo(ibValueMetaObjectAttributeBase* attribute) :
	ibValueModelColumnInfo(), m_metaAttribute(attribute)
{
}

ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnInfo::~ibValueDataObjectTreeColumnInfo()
{
}

//////////////////////////////////////////////////////////////////////
//					  ibValueDataObjectListReturnLine                     //
//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_DYNAMIC_CLASS(ibValueListDataObject::ibValueDataObjectListReturnLine, ibValueModelTable::ibValueModelReturnLine);

ibValueListDataObject::ibValueDataObjectListReturnLine::ibValueDataObjectListReturnLine(ibValueListDataObject* ownerTable, const ibDataViewItem& line) :
	ibValueModelReturnLine(line), m_methodHelper(new ibValueMethodHelper()), m_ownerTable(ownerTable)
{
}

ibValueListDataObject::ibValueDataObjectListReturnLine::~ibValueDataObjectListReturnLine()
{
	wxDELETE(m_methodHelper);
}

void ibValueListDataObject::ibValueDataObjectListReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	ibValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool ibValueListDataObject::ibValueDataObjectListReturnLine::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	return false;
}

bool ibValueListDataObject::ibValueDataObjectListReturnLine::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;
	const ibMetaID& id = m_methodHelper->GetPropData(lPropNum);
	wxValueTableRow* node = m_ownerTable->GetViewData<wxValueTableRow>(m_lineItem);
	if (node == nullptr)
		return false;
	return node->GetValue(id, pvarPropVal);
}

wxIMPLEMENT_DYNAMIC_CLASS(ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine, ibValueModelTree::ibValueModelReturnLine);

ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine::ibValueDataObjectTreeReturnLine(ibValueModelTreeDataObject* ownerTable, const ibDataViewItem& line) :
	ibValueModelReturnLine(line),
	m_methodHelper(new ibValueMethodHelper()), m_ownerTable(ownerTable)
{
}

ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine::~ibValueDataObjectTreeReturnLine()
{
	wxDELETE(m_methodHelper);
}

void ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	ibValueMetaObjectGenericData* metaObject = m_ownerTable->GetMetaObject();
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		m_methodHelper->AppendProp(
			object->GetName(),
			object->GetMetaID()
		);
	}
}

bool ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	return false;
}

bool ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (appData->DesignerMode())
		return false;
	const ibMetaID& id = m_methodHelper->GetPropData(lPropNum);
	wxValueTreeNode* node = m_ownerTable->GetViewData<wxValueTreeNode>(m_lineItem);
	if (node != nullptr) {
		return node->GetValue(id, pvarPropVal);
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

ibDataViewItem ibValueListDataObjectEnumRef::FindRowValue(const ibValue& varValue, const wxString& colName) const
{
	ibValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		for (long row = 0; row < GetRowCount(); row++) {
			ibDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetGuid())
					return item;
			}
		}
	}
	return ibDataViewItem(nullptr);
}

ibDataViewItem ibValueListDataObjectEnumRef::FindRowValue(ibValueModelReturnLine* retLine) const
{
	wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node](wxValueTableRow* row)
		{
			return node->GetGuid() == ((wxValueTableEnumRow*)row)->GetGuid();
		}
	);
	if (it != m_nodeValues.end()) return ibDataViewItem(*it);
	return ibDataViewItem(nullptr);
}

ibValueListDataObjectEnumRef::ibValueListDataObjectEnumRef(ibValueMetaObjectRecordDataEnumRef* metaObject, const ibFormID& formType, bool choiceMode) :
	ibValueListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
	ibValueListDataObject::AppendSort(m_metaObject->GetDataOrder(), true, true, true);
	ibValueListDataObject::AppendSort(m_metaObject->GetDataReference(), true, true, true);
}

CSourceExplorer ibValueListDataObjectEnumRef::GetSourceExplorer() const
{
	CSourceExplorer srcHelper(
		m_metaObject, GetClassType(),
		true, true
	);
	srcHelper.AppendSource(m_metaObject->GetDataReference(), true, true);
	return srcHelper;
}

bool ibValueListDataObjectEnumRef::GetModel(ibValueModel*& tableValue, const ibMetaID& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void ibValueListDataObjectEnumRef::ChooseValue(ibBackendValueForm* srcForm)
{
	wxValueTableEnumRow* node = GetViewData<wxValueTableEnumRow>(GetSelection());
	if (node == nullptr)
		return;
	wxASSERT(srcForm);
	ibValueReferenceDataObject* dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
	if (dataValueRef != nullptr) {
		srcForm->NotifyChoice(dataValueRef->GetValue());
	}
}

#include "backend/objCtor.h"

ibClassID ibValueListDataObjectEnumRef::GetClassType() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString ibValueListDataObjectEnumRef::GetClassName() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString ibValueListDataObjectEnumRef::GetString() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

ibDataViewItem ibValueListDataObjectRef::FindRowValue(const ibValue& varValue, const wxString& colName) const
{
	ibValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		for (long row = 0; row < GetRowCount(); row++) {
			ibDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableListRow* node = GetViewData<wxValueTableListRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetGuid())
					return item;
			}
		}
	}
	return ibDataViewItem(nullptr);
}

ibDataViewItem ibValueListDataObjectRef::FindRowValue(ibValueModelReturnLine* retLine) const
{
	wxValueTableListRow* node = GetViewData<wxValueTableListRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node](wxValueTableRow* row)
		{
			return node->GetGuid() == ((wxValueTableListRow*)row)->GetGuid();
		}
	);
	if (it != m_nodeValues.end()) return ibDataViewItem(*it);
	return ibDataViewItem(nullptr);
}

ibValueListDataObjectRef::ibValueListDataObjectRef(ibValueMetaObjectRecordDataMutableRef* metaObject, const ibFormID& formType, bool choiceMode) :
	ibValueListDataObject(metaObject, formType, choiceMode), m_metaObject(metaObject), m_choiceMode(choiceMode)
{
}

CSourceExplorer ibValueListDataObjectRef::GetSourceExplorer() const
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

bool ibValueListDataObjectRef::GetModel(ibValueModel*& tableValue, const ibMetaID& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void ibValueListDataObjectRef::AddValue(unsigned int before)
{
	ibValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		try {
			ibValuePtr<ibValueRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListDataObjectRef::CopyValue()
{
	ibValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			ibValuePtr<ibValueRecordDataObjectRef> dataValueObject(metaObject->CopyObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListDataObjectRef::EditValue()
{
	ibValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			ibValueRecordDataObjectRef* dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr) dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListDataObjectRef::DeleteValue()
{
	ibValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			ibValuePtr<ibValueRecordDataObjectRef> dataValueObject(metaObject->CreateObjectValue(node->GetGuid()));
			if (dataValueObject != nullptr)
				dataValueObject->DeleteObject();
			ibBackendValueForm* valueListForm = ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr)
				valueListForm->UpdateForm();
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListDataObjectRef::MarkAsDeleteValue()
{
	ibValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
		if (node == nullptr)
			return;

		try {
			ibValuePtr<ibValueRecordDataObjectRef> dataValueObject = metaObject->CreateObjectValue(node->GetGuid());
			if (dataValueObject != nullptr)
				dataValueObject->SetDeletionMark(true);
			ibBackendValueForm* valueListForm = ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr)
				valueListForm->UpdateForm();
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListDataObjectRef::ChooseValue(ibBackendValueForm* srcForm)
{
	wxValueTableListRow* node = GetViewData<wxValueTableListRow>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	try {
		ibValuePtr<ibValueReferenceDataObject> dataValueRef = m_metaObject->FindObjectValue(node->GetGuid());
		if (dataValueRef != nullptr)
			srcForm->NotifyChoice(dataValueRef->GetValue());
	}
	catch (const ibBackendCoreException* err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

ibClassID ibValueListDataObjectRef::GetClassType() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString ibValueListDataObjectRef::GetClassName() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString ibValueListDataObjectRef::GetString() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

ibDataViewItem ibValueModelTreeDataObjectFolderRef::FindRowValue(const ibValue& varValue, const wxString& colName) const
{
	ibValueReferenceDataObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const ibGuid&)> findGuid = [&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const ibGuid& guid)
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
			return ibDataViewItem(foundedNode);
	}
	return ibDataViewItem(nullptr);
}

ibDataViewItem ibValueModelTreeDataObjectFolderRef::FindRowValue(ibValueModelReturnLine* retLine) const
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(retLine->GetLineItem());
	std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const ibGuid&)> findGuid =
		[&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const ibGuid& guid)
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
	if (foundedNode != nullptr) return ibDataViewItem(foundedNode);
	return ibDataViewItem(nullptr);
}

ibValueModelTreeDataObjectFolderRef::ibValueModelTreeDataObjectFolderRef(ibValueMetaObjectRecordDataHierarchyMutableRef* metaObject, const ibFormID& formType,
	int listMode, bool choiceMode) : ibValueModelTreeDataObject(metaObject, formType, choiceMode),
	m_metaObject(metaObject), m_listMode(listMode), m_choiceMode(choiceMode)
{
	//ibValueModelTreeDataObject::AppendSort(m_metaObject->GetDataIsFolder(), false, true, true);
	ibValueModelTreeDataObject::AppendSort(m_metaObject->GetDataCode(), true, false);
	ibValueModelTreeDataObject::AppendSort(m_metaObject->GetDataDescription(), true);
	ibValueModelTreeDataObject::AppendSort(m_metaObject->GetDataReference());
}

CSourceExplorer ibValueModelTreeDataObjectFolderRef::GetSourceExplorer() const
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

bool ibValueModelTreeDataObjectFolderRef::GetModel(ibValueModel*& tableValue, const ibMetaID& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void ibValueModelTreeDataObjectFolderRef::AddValue(unsigned int before)
{
	ibValue cParent; ibValue isFolder = true;

	if (m_topParentGuid.isValid()) {
		cParent = ibValueReferenceDataObject::Create(m_metaObject, m_topParentGuid);
	}
	else {
		wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
		if (node != nullptr) {
			node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);
			if (!isFolder.GetBoolean())
				node->GetValue(*m_metaObject->GetDataParent(), cParent);
			else
				node->GetValue(*m_metaObject->GetDataReference(), cParent);
		}
	}

	ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject(m_metaObject->CreateObjectValue(ibObjectMode::OBJECT_ITEM));

	if (dataValueFolderObject != nullptr) {
		dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
		dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
}

void ibValueModelTreeDataObjectFolderRef::AddFolderValue(unsigned int before)
{
	ibValue cParent; ibValue isFolder = true;

	if (m_topParentGuid.isValid()) {
		cParent = ibValueReferenceDataObject::Create(m_metaObject, m_topParentGuid);
	}
	else {
		wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
		if (node != nullptr) {
			node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);
			if (!isFolder.GetBoolean())
				node->GetValue(*m_metaObject->GetDataParent(), cParent);
			else
				node->GetValue(*m_metaObject->GetDataReference(), cParent);
		}
	}

	try {
		ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject(m_metaObject->CreateObjectValue(ibObjectMode::OBJECT_FOLDER));
		if (dataValueFolderObject != nullptr) {
			dataValueFolderObject->SetValueByMetaID(*m_metaObject->GetDataParent(), cParent);
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
	}
	catch (const ibBackendCoreException* err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

void ibValueModelTreeDataObjectFolderRef::CopyValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	ibValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject = m_metaObject->CopyObjectValue(isFolder.GetBoolean() ? ibObjectMode::OBJECT_FOLDER : ibObjectMode::OBJECT_ITEM, node->GetGuid());
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (const ibBackendCoreException* err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

void ibValueModelTreeDataObjectFolderRef::EditValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	ibValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? ibObjectMode::OBJECT_FOLDER : ibObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr) dataValueFolderObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
	}
	catch (const ibBackendCoreException* err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

void ibValueModelTreeDataObjectFolderRef::DeleteValue()
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	ibValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

	try {
		ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject(m_metaObject->CreateObjectValue(isFolder.GetBoolean() ? ibObjectMode::OBJECT_FOLDER : ibObjectMode::OBJECT_ITEM, node->GetGuid()));
		if (dataValueFolderObject != nullptr)
			dataValueFolderObject->DeleteObject();
		ibBackendValueForm* valueListForm = ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
		if (valueListForm != nullptr) valueListForm->UpdateForm();
	}
	catch (const ibBackendCoreException *err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

void ibValueModelTreeDataObjectFolderRef::MarkAsDeleteValue()
{
	ibValueMetaObjectRecordDataHierarchyMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
		if (node == nullptr)
			return;

		ibValue isFolder = false;
		node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);

		try {
			ibValuePtr<ibValueRecordDataObjectHierarchyRef> dataValueFolderObject(metaObject->CreateObjectValue(isFolder.GetBoolean() ? ibObjectMode::OBJECT_FOLDER : ibObjectMode::OBJECT_ITEM, node->GetGuid()));
			if (dataValueFolderObject != nullptr)
				dataValueFolderObject->SetDeletionMark(true);
			ibBackendValueForm* valueListForm = ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
			if (valueListForm != nullptr) valueListForm->UpdateForm();
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueModelTreeDataObjectFolderRef::ChooseValue(ibBackendValueForm* srcForm)
{
	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(GetSelection());
	if (node == nullptr)
		return;

	wxASSERT(srcForm);

	ibValue cIsFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), cIsFolder);

	try {
		ibValuePtr<ibValueReferenceDataObject> dataValueFolderRef(m_metaObject->FindObjectValue(node->GetGuid()));
		if (m_listMode == LIST_FOLDER && cIsFolder.GetBoolean())
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
		else if (m_listMode == LIST_ITEM && !cIsFolder.GetBoolean())
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
		else if (m_listMode == LIST_ITEM_FOLDER)
			srcForm->NotifyChoice(dataValueFolderRef->GetValue());
	}
	catch (const ibBackendCoreException* err) {
		ibValueSystemFunction::Alert(err->GetErrorDescription());
	}
	catch (...) {
	}
}

#include "backend/objCtor.h"

ibClassID ibValueModelTreeDataObjectFolderRef::GetClassType() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString ibValueModelTreeDataObjectFolderRef::GetClassName() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString ibValueModelTreeDataObjectFolderRef::GetString() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

/////////////////////////////////////////////////////////////////////////////////////////////

ibDataViewItem ibValueListRegisterObject::FindRowValue(const ibValue& varValue, const wxString& colName) const
{
	ibValueRecordManagerObject* pRefData = nullptr;
	if (varValue.ConvertToValue(pRefData)) {
		ibValueMetaObjectRegisterData* metaObject = GetMetaObject();
		wxASSERT(metaObject);
		for (long row = 0; row < GetRowCount(); row++) {
			ibDataViewItem item = GetItem(row);
			if (item.IsOk()) {
				wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(item);
				if (node != nullptr && pRefData->GetGuid() == node->GetUniquePairKey(metaObject))
					return item;
			}
		}
	}

	return ibDataViewItem(nullptr);
}

ibDataViewItem ibValueListRegisterObject::FindRowValue(ibValueModelReturnLine* retLine) const
{
	ibValueMetaObjectRegisterData* metaObject = GetMetaObject();
	wxASSERT(metaObject);
	wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(retLine->GetLineItem());
	auto it = std::find_if(m_nodeValues.begin(), m_nodeValues.end(), [node, metaObject](wxValueTableRow* row)
		{
			return node->GetUniquePairKey(metaObject) == ((wxValueTableKeyRow*)row)->GetUniquePairKey(metaObject);
		}
	);
	if (it != m_nodeValues.end()) return ibDataViewItem(*it);
	return ibDataViewItem(nullptr);
}

ibValueListRegisterObject::ibValueListRegisterObject(ibValueMetaObjectRegisterData* metaObject, const ibFormID& formType) :
	ibValueListDataObject(metaObject, formType), m_metaObject(metaObject)
{
	if (m_metaObject->HasRecorder()) {
		if (m_metaObject->HasPeriod()) ibValueListDataObject::AppendSort(metaObject->GetRegisterPeriod());
		ibValueListDataObject::AppendSort(metaObject->GetRegisterRecorder());
		ibValueListDataObject::AppendSort(metaObject->GetRegisterLineNumber());
	}
	else if (m_metaObject->HasPeriod()) {
		ibValueListDataObject::AppendSort(metaObject->GetRegisterPeriod());
	}

	for (auto& dimension : m_metaObject->GetGenericDimentionArrayObject()) {
		ibValueListDataObject::AppendSort(dimension, true, true, true);
	}
}

CSourceExplorer ibValueListRegisterObject::GetSourceExplorer() const
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

bool ibValueListRegisterObject::GetModel(ibValueModel*& tableValue, const ibMetaID& id)
{
	if (id == m_metaObject->GetMetaID()) {
		tableValue = this;
		return true;
	}
	return false;
}

//events 
void ibValueListRegisterObject::AddValue(unsigned int before)
{
	if (m_metaObject != nullptr && m_metaObject->HasRecordManager()) {
		try {
			ibValuePtr<ibValueRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue());
			if (dataValueObject != nullptr)
				dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
		}
		catch (const ibBackendCoreException* err) {
			ibValueSystemFunction::Alert(err->GetErrorDescription());
		}
		catch (...) {
		}
	}
}

void ibValueListRegisterObject::CopyValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				ibValuePtr<ibValueRecordManagerObject> dataValueObject(m_metaObject->CopyRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (const ibBackendCoreException* err) {
				ibValueSystemFunction::Alert(err->GetErrorDescription());
			}
			catch (...) {
			}
		}
	}
}

void ibValueListRegisterObject::EditValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				ibValuePtr<ibValueRecordManagerObject> dataValueObject(m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject)));
				if (dataValueObject != nullptr)
					dataValueObject->ShowFormValue(wxEmptyString, dynamic_cast<ibBackendControlFrame*>(ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid)));
			}
			catch (const ibBackendCoreException* err) {
				ibValueSystemFunction::Alert(err->GetErrorDescription());
			}
			catch (...) {
			}
		}
		else {
			try {
				const ibValueMetaObjectAttributePredefined* metaRecorder = m_metaObject->GetRegisterRecorder();
				if (metaRecorder != nullptr) {
					ibValue recorderVal = node->GetTableValue(metaRecorder->GetMetaID());
					recorderVal.ShowValue();
				}
			}
			catch (const ibBackendCoreException* err) {
				ibValueSystemFunction::Alert(err->GetErrorDescription());
			}
			catch (...) {
			}
		}
	}
}

void ibValueListRegisterObject::DeleteValue()
{
	if (m_metaObject != nullptr) {
		wxValueTableKeyRow* node = GetViewData<wxValueTableKeyRow>(GetSelection());
		if (node == nullptr) return;
		if (m_metaObject->HasRecordManager()) {
			try {
				ibValuePtr<ibValueRecordManagerObject> dataValueObject = m_metaObject->CreateRecordManagerObjectValue(node->GetUniquePairKey(m_metaObject));
				if (dataValueObject != nullptr)
					dataValueObject->DeleteRegister();
				ibBackendValueForm* valueListForm = ibBackendValueForm::FindFormBySourceUniqueKey(m_objGuid);
				if (valueListForm != nullptr) valueListForm->UpdateForm();
			}
			catch (const ibBackendCoreException* err) {
				ibValueSystemFunction::Alert(err->GetErrorDescription());
			}
			catch (...) {
			}
		}
	}
}

ibClassID ibValueListRegisterObject::GetClassType() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString ibValueListRegisterObject::GetClassName() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

wxString ibValueListRegisterObject::GetString() const
{
	const ibCtorMetaValueType* clsFactory =
		m_metaObject->GetTypeCtor(ibCtorMetaType::ibCtorMetaType_List);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void ibValueListDataObjectEnumRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool ibValueListDataObjectEnumRef::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
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

void ibValueListDataObjectRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool ibValueListDataObjectRef::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
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

void ibValueModelTreeDataObjectFolderRef::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProp(wxT("ChoiceMode"));
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool ibValueModelTreeDataObjectFolderRef::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
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

void ibValueListRegisterObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendProc(wxT("Refresh"), wxT("Refresh()"));
}

bool ibValueListRegisterObject::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
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

bool ibValueListDataObjectEnumRef::SetPropVal(const long lPropNum, const ibValue& value) //setting attribute
{
	return false;
}

bool ibValueListDataObjectEnumRef::GetPropVal(const long lPropNum, ibValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool ibValueListDataObjectRef::SetPropVal(const long lPropNum, const ibValue& value) //setting attribute
{
	return false;
}

bool ibValueListDataObjectRef::GetPropVal(const long lPropNum, ibValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool ibValueModelTreeDataObjectFolderRef::SetPropVal(const long lPropNum, const ibValue& value) //setting attribute
{
	return false;
}

bool ibValueModelTreeDataObjectFolderRef::GetPropVal(const long lPropNum, ibValue& pvarPropVal) //attribute value
{
	if (lPropNum == enChoiceMode) {
		pvarPropVal = m_choiceMode;
		return true;
	}
	return false;
}

bool ibValueListRegisterObject::SetPropVal(const long lPropNum, const ibValue& value) //setting attribute
{
	return false;
}

bool ibValueListRegisterObject::GetPropVal(const long lPropNum, ibValue& pvarPropVal) //attribute value
{
	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(ibValueListDataObject::ibValueDataObjectListColumnCollection, "ListColumn", string_to_clsid("VL_LVC"));
SYSTEM_TYPE_REGISTER(ibValueListDataObject::ibValueDataObjectListColumnCollection::ibValueDataObjectListColumnInfo, "ListColumnInfo", string_to_clsid("VL_LCI"));
SYSTEM_TYPE_REGISTER(ibValueListDataObject::ibValueDataObjectListReturnLine, "ListValueRow", string_to_clsid("VL_LVR"));

SYSTEM_TYPE_REGISTER(ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection, "TreeColumn", string_to_clsid("VL_TVC"));
SYSTEM_TYPE_REGISTER(ibValueModelTreeDataObject::ibValueDataObjectTreeColumnCollection::ibValueDataObjectTreeColumnInfo, "TreeColumnInfo", string_to_clsid("VL_TCI"));
SYSTEM_TYPE_REGISTER(ibValueModelTreeDataObject::ibValueDataObjectTreeReturnLine, "TreeValueRow", string_to_clsid("VL_TVR"));