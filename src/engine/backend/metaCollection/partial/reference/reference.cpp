////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : reference object
////////////////////////////////////////////////////////////////////////////

#include "reference.h"
#include "backend/metaData.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaCollection/partial/tabularSection/tabularSection.h"
#include "backend/databaseLayer/databaseLayer.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueReferenceDataObject, CValue);

//**********************************************************************************************
//*                                     reference                                              *        
//**********************************************************************************************
//static std::vector <CValueReferenceDataObject*> gs_references;
//**********************************************************************************************

void CValueReferenceDataObject::PrepareRef(bool createData)
{
	wxASSERT(m_metaObject != nullptr);

	if (m_initializedRef)
		return;

	if (CValueReferenceDataObject::IsEmpty()) {
		//attrbutes can refValue 
		for (const auto object : m_metaObject->GetGenericAttributeArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!m_metaObject->IsDataReference(object->GetMetaID())) {
				m_listObjectValue.insert_or_assign(object->GetMetaID(), object->CreateValue());
			}
		}
		// table is collection values 
		for (const auto object : m_metaObject->GetTableArrayObject()) {
			if (object->IsDeleted())
				continue;
			m_listObjectValue.insert_or_assign(object->GetMetaID(),
				CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, object));
		}
	}
	else if (CValueReferenceDataObject::ReadData(createData)) {
		m_foundedRef = true; m_newObject = false;
	}

	if (createData) {
		m_initializedRef = true;
	}

	PrepareNames();
}

CValueReferenceDataObject::CValueReferenceDataObject(IValueMetaObjectRecordDataRef* metaObject, const CGuid& objGuid) : CValue(eValueTypes::TYPE_VALUE, true), IValueDataObject(objGuid, !objGuid.isValid()),
m_metaObject(metaObject), m_methodHelper(new CMethodHelper()), m_initializedRef(false), m_reference_impl(nullptr), m_foundedRef(false)
{
	m_reference_impl = new reference_t(m_metaObject->GetMetaID(), m_objGuid);
	//gs_references.emplace_back(this);
}

CValueReferenceDataObject::~CValueReferenceDataObject()
{
	wxDELETE(m_reference_impl);
	//gs_references.erase(
	//	std::remove_if(gs_references.begin(), gs_references.end(),
	//		[this](CValueReferenceDataObject* ref) { return ref == this;}), gs_references.end()
	//);
	wxDELETE(m_methodHelper);
}

CValueReferenceDataObject* CValueReferenceDataObject::Create(IMetaData* metaData, const meta_identifier_t& id, const CGuid& objGuid)
{
	IValueMetaObjectRecordDataRef* metaObject = metaData->FindAnyObjectByFilter<IValueMetaObjectRecordDataRef>(id);
	if (metaObject != nullptr) {
		//auto& it = std::find_if(gs_references.begin(), gs_references.end(), [metaObject, objGuid](CValueReferenceDataObject* ref) {
		//	return metaObject == ref->GetMetaObject() && objGuid == ref->GetGuid(); }
		//);
		//if (it != gs_references.end())
		//	return *it;
		CValueReferenceDataObject* refData = new CValueReferenceDataObject(metaObject, objGuid);
		if (refData != nullptr)
			refData->PrepareRef(true);
		return refData;
	}
	return nullptr;
}

CValueReferenceDataObject* CValueReferenceDataObject::Create(IValueMetaObjectRecordDataRef* metaObject, const CGuid& objGuid)
{
	//auto& it = std::find_if(gs_references.begin(), gs_references.end(), [metaObject, objGuid](CValueReferenceDataObject* ref) {
	//	return metaObject == ref->GetMetaObject() && objGuid == ref->GetGuid(); }
	//);
	//if (it != gs_references.end())
	//	return *it;
	CValueReferenceDataObject* refData = new CValueReferenceDataObject(metaObject, objGuid);
	if (refData != nullptr)
		refData->PrepareRef(true);
	return refData;
}

CValueReferenceDataObject* CValueReferenceDataObject::Create(IMetaData* metaData, void* ptr)
{
	reference_t* reference = static_cast<reference_t*>(ptr);
	if (reference != nullptr) {
		IValueMetaObjectRecordDataRef* metaObject = metaData->FindAnyObjectByFilter<IValueMetaObjectRecordDataRef>(reference->m_id);
		if (metaObject != nullptr) {
			//auto& it = std::find_if(gs_references.begin(), gs_references.end(), [metaObject, reference](CValueReferenceDataObject* ref) {
			//	return metaObject == ref->GetMetaObject() && ref->GetGuid() == reference->m_guid; }
			//);
			//if (it != gs_references.end())
			//	return *it;
			return new CValueReferenceDataObject(metaObject, reference->m_guid);
		}
	}
	return nullptr;
}

CValueReferenceDataObject* CValueReferenceDataObject::CreateFromPtr(IMetaData* metaData, void* ptr)
{
	reference_t* reference = static_cast<reference_t*>(ptr);
	if (reference != nullptr) {
		IValueMetaObjectRecordDataRef* metaObject = metaData->FindAnyObjectByFilter<IValueMetaObjectRecordDataRef>(reference->m_id);
		if (metaObject != nullptr) {
			//auto& it = std::find_if(gs_references.begin(), gs_references.end(), [metaObject, reference](CValueReferenceDataObject* ref) {
			//	return metaObject == ref->GetMetaObject() && ref->GetGuid() == reference->m_guid; }
			//);
			//if (it != gs_references.end())
			//	return *it;
			CValueReferenceDataObject* refData = new CValueReferenceDataObject(metaObject, reference->m_guid);
			if (refData != nullptr)
				refData->PrepareRef(false);
			return refData;
		}
	}
	return nullptr;
}

CValueReferenceDataObject* CValueReferenceDataObject::CreateFromResultSet(IDatabaseResultSet* rs, IValueMetaObjectRecordDataRef* metaObject, const CGuid& refGuid)
{
	//auto& it = std::find_if(gs_references.begin(), gs_references.end(), [metaObject, refGuid](CValueReferenceDataObject* ref) {
	//	return metaObject == ref->GetMetaObject() && refGuid == ref->GetGuid(); }
	//);
	//if (it != gs_references.end())
	//	return *it;

	CValueReferenceDataObject* refData = new CValueReferenceDataObject(metaObject, refGuid);

	//load attributes 
	for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
		if (object->IsDeleted())
			continue;
		if (metaObject->IsDataReference(object->GetMetaID()))
			continue;
		IValueMetaObjectAttribute::GetValueAttribute(
			object,
			refData->m_listObjectValue[object->GetMetaID()],
			rs,
			false
		);
	}

	// table is collection values 
	for (const auto object : metaObject->GetTableArrayObject()) {
		if (object->IsDeleted())
			continue;
		refData->m_listObjectValue.insert_or_assign(
			object->GetMetaID(),
			CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(refData, object, true)
		);
	}

	refData->m_foundedRef = true;
	return refData;
}

bool CValueReferenceDataObject::SetValueByMetaID(const meta_identifier_t& id, const CValue& varMetaVal)
{
	return false;
}

bool CValueReferenceDataObject::GetValueByMetaID(const meta_identifier_t& id, CValue& pvarMetaVal) const
{
	if (m_metaObject->IsDataReference(id)) {
		if (!CValueReferenceDataObject::IsEmpty()) {
			pvarMetaVal = CValueReferenceDataObject::Create(m_metaObject, m_objGuid);
			return true;
		}
		pvarMetaVal = CValueReferenceDataObject::Create(m_metaObject);
		return true;
	}
	auto& it = m_listObjectValue.find(id);
	//wxASSERT(it != m_listObjectValue.end());
	if (it != m_listObjectValue.end()) {
		pvarMetaVal = it->second;
		return true;
	}
	return false;
}

void CValueReferenceDataObject::ShowValue()
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {
		IValueRecordDataObject* objValue = nullptr;
		if (metaObject != nullptr && m_objGuid.isValid())
			objValue = metaObject->CreateObjectValue(m_objGuid);
		else
			objValue = metaObject->CreateObjectValue();
		if (objValue != nullptr)
			objValue->ShowFormValue();
	}
}

IValueRecordDataObjectRef* CValueReferenceDataObject::GetObject() const
{
	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {
		if (m_newObject)
			return metaObject->CreateObjectValue();
		return metaObject->CreateObjectValue(m_objGuid);
	}
	return nullptr;
}

#include "backend/objCtor.h"

class_identifier_t CValueReferenceDataObject::GetClassType() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Reference);
	wxASSERT(clsFactory);
	return clsFactory->GetClassType();
}

wxString CValueReferenceDataObject::GetString() const
{
	if (m_newObject)
		return wxEmptyString;
	else if (!m_foundedRef)
		return wxString::Format("%s <%i:%s>", _("Not found"), m_metaObject->GetMetaID(), m_objGuid.str());

	wxASSERT(m_metaObject);
	return m_metaObject->GetDataPresentation(this);
}

wxString CValueReferenceDataObject::GetClassName() const
{
	const IMetaValueTypeCtor* clsFactory =
		m_metaObject->GetTypeCtor(eCtorMetaType::eCtorMetaType_Reference);
	wxASSERT(clsFactory);
	return clsFactory->GetClassName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

enum Func {
	enIsEmpty = 0,
	enGetMetadata,
	enGetObject,
	enGetGuid
};

void CValueReferenceDataObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	IValueMetaObjectRecordDataMutableRef* metaObject = nullptr;
	if (m_metaObject->ConvertToValue(metaObject)) {

		m_methodHelper->AppendFunc(wxT("IsEmpty"), wxT("IsEmpty()"));
		m_methodHelper->AppendFunc(wxT("GetMetadata"), wxT("GetMetadata()"));
		m_methodHelper->AppendFunc(wxT("GetObject"), wxT("GetObject()"));
		m_methodHelper->AppendFunc(wxT("GetGuid"), wxT("GetGuid()"));

		wxString objectName;

		//fill custom attributes 
		for (const auto object : metaObject->GetGenericAttributeArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				true,
				false,
				object->GetMetaID(),
				eProperty
			);
		}

		//fill custom tables 
		for (const auto object : metaObject->GetTableArrayObject()) {
			if (object->IsDeleted())
				continue;
			if (!object->GetObjectNameAsString(objectName))
				continue;
			m_methodHelper->AppendProp(
				objectName,
				true,
				false,
				object->GetMetaID(),
				eTable
			);
		}
	}
	else {
		m_methodHelper->AppendFunc(wxT("IsEmpty"), wxT("IsEmpty()"));
		m_methodHelper->AppendFunc(wxT("GetMetadata"), wxT("GetMetadata()"));
	}
}

bool CValueReferenceDataObject::SetPropVal(const long lPropNum, const CValue& value)
{
	return false;
}

bool CValueReferenceDataObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	CValueReferenceDataObject::PrepareRef();
	const meta_identifier_t& id = m_methodHelper->GetPropData(lPropNum);
	if (!m_metaObject->IsDataReference(id)) {
		if (lPropAlias == eTable && !GetValueByMetaID(id, pvarPropVal)) {
			m_listObjectValue.insert_or_assign(id,
				CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectRef>(this, m_metaObject->FindTableObjectByFilter(id), !m_newObject)
			);
		}
		if (lPropAlias == eTable && GetValueByMetaID(id, pvarPropVal)) {
			CValueTabularSectionDataObjectRef* tabularSection = nullptr;
			if (pvarPropVal.ConvertToValue(tabularSection)) {
				if (tabularSection->IsReadAfter()) {
					if (!tabularSection->LoadData(m_objGuid, false)) {
						pvarPropVal.Reset();
						return false;
					}
				}
				return true;
			}
			pvarPropVal.Reset();
			return false;
		}
		return GetValueByMetaID(id, pvarPropVal);
	}

	if (!CValueReferenceDataObject::IsEmpty()) {
		pvarPropVal = CValueReferenceDataObject::Create(m_metaObject, m_objGuid);
		return true;
	}

	pvarPropVal = CValueReferenceDataObject::Create(m_metaObject);
	return true;
}

bool CValueReferenceDataObject::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enIsEmpty:
		pvarRetValue = IsEmptyRef();
		return true;
	case enGetMetadata:
		pvarRetValue = GetMetaObject();
		return true;
	case enGetObject:
		pvarRetValue = GetObject();
		return true;
	case enGetGuid:
		pvarRetValue = CValue::CreateAndPrepareValueRef<CValueGuid>(m_objGuid);
		return true;
	}

	return false;
}