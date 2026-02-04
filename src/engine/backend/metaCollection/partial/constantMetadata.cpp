////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : constants
////////////////////////////////////////////////////////////////////////////

#include "constant.h"
#include "backend/metaData.h"

#define objectModule wxT("objectModule")

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectConstant, CValueMetaObjectAttribute)

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

CValueMetaObjectConstant::CValueMetaObjectConstant() : CValueMetaObjectAttribute()
{
	//set default proc
	m_propertyModule->GetMetaObject()->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	m_propertyModule->GetMetaObject()->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CValueMetaObjectConstant::~CValueMetaObjectConstant()
{
}

bool CValueMetaObjectConstant::LoadData(CMemoryReader& dataReader)
{
	//load object module
	m_propertyModule->GetMetaObject()->LoadMeta(dataReader);

	return CValueMetaObjectAttribute::LoadData(dataReader);
}

bool CValueMetaObjectConstant::SaveData(CMemoryWriter& dataWritter)
{
	//save object module
	m_propertyModule->GetMetaObject()->SaveMeta(dataWritter);

	return CValueMetaObjectAttribute::SaveData(dataWritter);
}

bool CValueMetaObjectConstant::DeleteData()
{
	return CValueMetaObjectAttribute::DeleteData();
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CValueMetaObjectConstant::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!CValueMetaObjectAttribute::OnCreateMetaObject(metaData, flags))
		return false;

	return m_propertyModule->GetMetaObject()->OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectConstant::OnLoadMetaObject(IMetaData* metaData)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!m_propertyModule->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	return CValueMetaObjectAttribute::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectConstant::OnSaveMetaObject(int flags)
{
	if (!m_propertyModule->GetMetaObject()->OnSaveMetaObject(flags))
		return false;

	return CValueMetaObjectAttribute::OnSaveMetaObject(flags);
}

bool CValueMetaObjectConstant::OnDeleteMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnDeleteMetaObject())
		return false;

	return CValueMetaObjectAttribute::OnDeleteMetaObject();
}

#include "backend/constantCtor.h"

bool CValueMetaObjectConstant::OnBeforeRunMetaObject(int flags)
{
	if (!m_propertyModule->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	registerConstObject();
	registerConstManager();

	return CValueMetaObjectAttribute::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectConstant::OnAfterRunMetaObject(int flags)
{
	if (!m_propertyModule->GetMetaObject()->OnAfterRunMetaObject(flags))
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (CValueMetaObjectAttribute::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModule->GetMetaObject(), CreateRecordDataObjectValue());

		return false;
	}

	return CValueMetaObjectAttribute::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectConstant::OnBeforeCloseMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnBeforeCloseMetaObject())
		return false;

	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (moduleManager->RemoveCompileModule(m_propertyModule->GetMetaObject()))
			return CValueMetaObjectAttribute::OnAfterCloseMetaObject();

		return false;
	}

	return CValueMetaObjectAttribute::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectConstant::OnAfterCloseMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	unregisterConstObject();
	unregisterConstManager();

	return CValueMetaObjectAttribute::OnAfterCloseMetaObject();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IBackendValueForm* CValueMetaObjectConstant::GetObjectForm()
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(nullptr, nullptr, m_metaGuid);
	if (foundedForm == nullptr)
		return IValueMetaObjectForm::CreateAndBuildForm(nullptr, nullptr, CreateRecordDataObjectValue(), m_metaGuid);
	return foundedForm;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectConstant, "constant", g_metaConstantCLSID);
