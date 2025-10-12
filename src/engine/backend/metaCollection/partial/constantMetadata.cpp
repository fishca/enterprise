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

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectConstant, CMetaObjectAttribute)

//***********************************************************************
//*                         Attributes                                  * 
//***********************************************************************

CMetaObjectConstant::CMetaObjectConstant() : CMetaObjectAttribute()
{
	//set default proc
	m_propertyModule->GetMetaObject()->SetDefaultProcedure("beforeWrite", eContentHelper::eProcedureHelper, { "cancel" });
	m_propertyModule->GetMetaObject()->SetDefaultProcedure("onWrite", eContentHelper::eProcedureHelper, { "cancel" });
}

CMetaObjectConstant::~CMetaObjectConstant()
{
}

bool CMetaObjectConstant::LoadData(CMemoryReader& dataReader)
{
	//load object module
	m_propertyModule->GetMetaObject()->LoadMeta(dataReader);

	return CMetaObjectAttribute::LoadData(dataReader);
}

bool CMetaObjectConstant::SaveData(CMemoryWriter& dataWritter)
{
	//save object module
	m_propertyModule->GetMetaObject()->SaveMeta(dataWritter);

	return CMetaObjectAttribute::SaveData(dataWritter);
}

bool CMetaObjectConstant::DeleteData()
{
	return CMetaObjectAttribute::DeleteData();
}

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

#include "backend/appData.h"

bool CMetaObjectConstant::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	if (!CMetaObjectAttribute::OnCreateMetaObject(metaData, flags))
		return false;

	return m_propertyModule->GetMetaObject()->OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectConstant::OnLoadMetaObject(IMetaData* metaData)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!m_propertyModule->GetMetaObject()->OnLoadMetaObject(metaData))
		return false;

	return CMetaObjectAttribute::OnLoadMetaObject(metaData);
}

bool CMetaObjectConstant::OnSaveMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnSaveMetaObject())
		return false;

	return CMetaObjectAttribute::OnSaveMetaObject();
}

bool CMetaObjectConstant::OnDeleteMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnDeleteMetaObject())
		return false;

	return CMetaObjectAttribute::OnDeleteMetaObject();
}

#include "backend/objCtor.h"

bool CMetaObjectConstant::OnBeforeRunMetaObject(int flags)
{
	if (!m_propertyModule->GetMetaObject()->OnBeforeRunMetaObject(flags))
		return false;

	registerConstObject();
	registerManager();

	return CMetaObjectAttribute::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectConstant::OnAfterRunMetaObject(int flags)
{
	if (!m_propertyModule->GetMetaObject()->OnAfterRunMetaObject(flags))
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (CMetaObjectAttribute::OnAfterRunMetaObject(flags))
			return moduleManager->AddCompileModule(m_propertyModule->GetMetaObject(), CreateObjectValue());

		return false;
	}

	return CMetaObjectAttribute::OnAfterRunMetaObject(flags);
}

bool CMetaObjectConstant::OnBeforeCloseMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnBeforeCloseMetaObject())
		return false;

	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (appData->DesignerMode()) {

		if (moduleManager->RemoveCompileModule(m_propertyModule->GetMetaObject()))
			return CMetaObjectAttribute::OnAfterCloseMetaObject();

		return false;
	}

	return CMetaObjectAttribute::OnBeforeCloseMetaObject();
}

bool CMetaObjectConstant::OnAfterCloseMetaObject()
{
	if (!m_propertyModule->GetMetaObject()->OnAfterCloseMetaObject())
		return false;

	unregisterConstObject();
	unregisterManager();

	return CMetaObjectAttribute::OnAfterCloseMetaObject();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IBackendValueForm* CMetaObjectConstant::GetObjectForm()
{
	IBackendValueForm* const foundedForm = IBackendValueForm::FindFormByUniqueKey(nullptr, nullptr, m_metaGuid);
	if (foundedForm == nullptr)
		return IMetaObjectForm::CreateAndBuildForm(nullptr, nullptr, CreateObjectValue(), m_metaGuid);
	return foundedForm;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectConstant, "constant", g_metaConstantCLSID);
