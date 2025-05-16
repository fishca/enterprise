#include "accumulationRegister.h"
#include "list/objectList.h"
#include "backend/metadataConfiguration.h"
#include "backend/moduleManager/moduleManager.h"

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectAccumulationRegister, IMetaObjectRegisterData);

/////////////////////////////////////////////////////////////////////////

CMetaObjectAccumulationRegister::CMetaObjectAccumulationRegister() : IMetaObjectRegisterData()
{
	//set default proc
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	m_propertyModuleObject->GetMetaObject()->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CMetaObjectAccumulationRegister::~CMetaObjectAccumulationRegister()
{
	wxDELETE(m_attributeRecordType);
}

CMetaObjectForm* CMetaObjectAccumulationRegister::GetDefaultFormByID(const form_identifier_t& id)
{
	if (id == eFormList && m_propertyDefFormList->GetValueAsInteger() != wxNOT_FOUND) {
		for (auto& obj : GetObjectForms()) {
			if (m_propertyDefFormList->GetValueAsInteger() == obj->GetMetaID()) {
				return obj;
			}
		}
	}

	return nullptr;
}



IBackendValueForm* CMetaObjectAccumulationRegister::GetListForm(const wxString& formName, IBackendControlFrame* ownerControl, const CUniqueKey& formGuid)
{
	CMetaObjectForm* defList = nullptr;

	if (!formName.IsEmpty()) {
		for (auto metaForm : GetObjectForms()) {
			if (CMetaObjectAccumulationRegister::eFormList == metaForm->GetTypeForm()
				&& stringUtils::CompareString(formName, metaForm->GetName())) {
				defList = metaForm; break;
			}
		}
		wxASSERT(defList);
	}
	else {
		defList = GetDefaultFormByID(CMetaObjectAccumulationRegister::eFormList);
	}

	if (defList == nullptr) {
		IBackendValueForm* valueForm = IBackendValueForm::CreateNewForm(ownerControl, nullptr,
			m_metaData->CreateAndConvertObjectValueRef<CListRegisterObject>(this, CMetaObjectAccumulationRegister::eFormList), formGuid
		);
		valueForm->BuildForm(CMetaObjectAccumulationRegister::eFormList);
		return valueForm;
	}

	return defList->GenerateFormAndRun(
		ownerControl, m_metaData->CreateAndConvertObjectValueRef<CListRegisterObject>(this, defList->GetTypeForm()), formGuid
	);

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

bool CMetaObjectAccumulationRegister::GetFormList(CPropertyList* prop)
{
	prop->AppendItem(_("<not selected>"), wxNOT_FOUND, wxEmptyValue);
	for (auto formObject : GetObjectForms()) {
		if (!formObject->IsAllowed()) continue;
		if (eFormList == formObject->GetTypeForm()) {
			prop->AppendItem(formObject->GetName(), formObject->GetMetaID(), formObject);
		}
	}
	return true; 
}

//***************************************************************************
//*                       Save & load metaData                              *
//***************************************************************************

bool CMetaObjectAccumulationRegister::LoadData(CMemoryReader& dataReader)
{
	//load default attributes:
	m_attributeRecordType->LoadMeta(dataReader);

	//load default form 
	m_propertyDefFormList->SetValue(GetIdByGuid(dataReader.r_stringZ()));

	//load data 
	m_propertyRegisterType->SetValue(dataReader.r_u16());

	//load object module
	m_propertyModuleObject->GetMetaObject()->LoadMeta(dataReader);
	m_propertyModuleManager->GetMetaObject()->LoadMeta(dataReader);

	return IMetaObjectRegisterData::LoadData(dataReader);
}

bool CMetaObjectAccumulationRegister::SaveData(CMemoryWriter& dataWritter)
{
	//save default attributes:
	m_attributeRecordType->SaveMeta(dataWritter);

	//save default form 
	dataWritter.w_stringZ(GetGuidByID(m_propertyDefFormList->GetValueAsInteger()));

	//save data
	dataWritter.w_u16(m_propertyRegisterType->GetValueAsInteger());

	//Save object module
	m_propertyModuleObject->GetMetaObject()->SaveMeta(dataWritter);
	m_propertyModuleManager->GetMetaObject()->SaveMeta(dataWritter);

	//create or update table:
	return IMetaObjectRegisterData::SaveData(dataWritter);
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CMetaObjectAccumulationRegister::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!IMetaObjectRegisterData::OnCreateMetaObject(metaData, flags))
		return false;

	return m_attributeRecordType->OnCreateMetaObject(metaData, flags) &&
		m_propertyModuleManager->GetMetaObject()->OnCreateMetaObject(metaData, flags) &&
		m_propertyModuleObject->GetMetaObject()->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectAccumulationRegister::OnLoadMetaObject(IMetaData* metaData)
{
	if (!m_attributeRecordType->OnLoadMetaObject(metaData))
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	return IMetaObjectRegisterData::OnLoadMetaObject(metaData);
}

bool CMetaObjectAccumulationRegister::OnSaveMetaObject()
{
	if (!m_attributeRecordType->OnSaveMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnSaveMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnSaveMetaObject())
		return false;

#if _USE_SAVE_METADATA_IN_TRANSACTION == 1
	if (!(m_attributeRecorder->GetClsidCount() > 0))
		return false;
#endif 

	return IMetaObjectRegisterData::OnSaveMetaObject();
}

bool CMetaObjectAccumulationRegister::OnDeleteMetaObject()
{
	if (!m_attributeRecordType->OnDeleteMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnDeleteMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnDeleteMetaObject())
		return false;

	return IMetaObjectRegisterData::OnDeleteMetaObject();
}

bool CMetaObjectAccumulationRegister::OnReloadMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {
		CRecordSetObjectAccumulationRegister* recordSet = nullptr;
		if (moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), recordSet)) {
			if (!recordSet->InitializeObject())
				return false;
		}
	}

	return true;
}

#include "backend/objCtor.h"

bool CMetaObjectAccumulationRegister::OnBeforeRunMetaObject(int flags)
{
	if (!m_attributeRecordType->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	registerSelection();

	return IMetaObjectRegisterData::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectAccumulationRegister::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnAfterRunMetaObject(flags)) {

			if (!moduleManager->AddCompileModule(m_propertyModuleObject->GetMetaObject(), CreateRecordSetObjectValue()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnAfterRunMetaObject(flags);
}

bool CMetaObjectAccumulationRegister::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (IMetaObjectRegisterData::OnBeforeCloseMetaObject()) {

			if (!moduleManager->RemoveCompileModule(m_propertyModuleObject->GetMetaObject()))
				return false;

			return true;
		}
	}

	return IMetaObjectRegisterData::OnBeforeCloseMetaObject();
}

bool CMetaObjectAccumulationRegister::OnAfterCloseMetaObject()
{
	if (!m_attributeRecordType->OnAfterCloseMetaObject())
		return false;

	if (!m_propertyModuleManager->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	if (!m_propertyModuleObject->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	unregisterSelection();

	return IMetaObjectRegisterData::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                             form events                             *
//***********************************************************************

void CMetaObjectAccumulationRegister::OnCreateFormObject(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == wxNOT_FOUND)
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

void CMetaObjectAccumulationRegister::OnRemoveMetaForm(IMetaObjectForm* metaForm)
{
	if (metaForm->GetTypeForm() == CMetaObjectAccumulationRegister::eFormList
		&& m_propertyDefFormList->GetValueAsInteger() == metaForm->GetMetaID())
	{
		m_propertyDefFormList->SetValue(metaForm->GetMetaID());
	}
}

std::vector<IMetaObjectAttribute*> CMetaObjectAccumulationRegister::GetDefaultAttributes() const
{
	std::vector<IMetaObjectAttribute*> attributes;
	attributes.push_back(m_attributeLineActive);
	if (GetRegisterType() == eRegisterType::eBalances) {
		attributes.push_back(m_attributeRecordType);
	}
	attributes.push_back(m_attributePeriod);
	attributes.push_back(m_attributeRecorder);
	attributes.push_back(m_attributeLineNumber);
	return attributes;
}

std::vector<IMetaObjectAttribute*> CMetaObjectAccumulationRegister::GetGenericDimensions() const
{
	std::vector<IMetaObjectAttribute*> attributes;
	attributes.push_back(m_attributeRecorder);
	return attributes;
}

ISourceDataObject* CMetaObjectAccumulationRegister::CreateObjectData(IMetaObjectForm* metaObject)
{
	switch (metaObject->GetTypeForm())
	{
	case eFormList:
		return 	m_metaData->CreateAndConvertObjectValueRef<CListRegisterObject>(this, metaObject->GetTypeForm());
	}

	return nullptr;
}

IRecordSetObject* CMetaObjectAccumulationRegister::CreateRecordSetObjectRegValue(const CUniquePairKey& uniqueKey)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);
	if (appData->DesignerMode()) {
		IRecordSetObject* pDataRef = nullptr;
		if (!moduleManager->FindCompileModule(m_propertyModuleObject->GetMetaObject(), pDataRef)) {
			return 	m_metaData->CreateAndConvertObjectValueRef<CRecordSetObjectAccumulationRegister>(this, uniqueKey);
		}
		return pDataRef;
	}
	return 	m_metaData->CreateAndConvertObjectValueRef<CRecordSetObjectAccumulationRegister>(this, uniqueKey);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectAccumulationRegister, "accumulationRegister", g_metaAccumulationRegisterCLSID);