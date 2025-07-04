////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metamodule object
////////////////////////////////////////////////////////////////////////////

#include "metaModuleObject.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

//***********************************************************************
//*                           ModuleObject                              *
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectModule, IMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectModule, IMetaObjectModule);
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectCommonModule, IMetaObjectModule);
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectManagerModule, CMetaObjectCommonModule);

//***********************************************************************
//*                           System metaData                           *
//***********************************************************************

#include "backend/debugger/debugServer.h"
#include "backend/debugger/debugClient.h"
#include "backend/metaData.h"

bool IMetaObjectModule::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IMetaObject::OnCreateMetaObject(metaData, flags);
}

bool IMetaObjectModule::OnLoadMetaObject(IMetaData* metaData)
{
	return IMetaObject::OnLoadMetaObject(metaData);
}

bool IMetaObjectModule::OnSaveMetaObject()
{
	//initialize debugger server
	if (appData->DesignerMode()) {
		return debugClient->SaveBreakpoints(GetDocPath());
	}

	return IMetaObject::OnSaveMetaObject();
}

bool IMetaObjectModule::OnDeleteMetaObject()
{
	return IMetaObject::OnDeleteMetaObject();
}

bool IMetaObjectModule::OnBeforeRunMetaObject(int flags)
{
	wxString moduleData = GetModuleText();
	//initialize debugger server
	unsigned int nNumber = 1 + moduleData.Replace('\n', '\n');
	if (appData->DesignerMode()) {
		debugClient->InitializeBreakpoints(GetDocPath(), 0, nNumber);
	}
	else {
		debugServer->InitializeBreakpoints(GetDocPath(), 0, nNumber);
	}
	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectModule::OnAfterCloseMetaObject()
{
	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                          default procedures						    *
//***********************************************************************

void IMetaObjectModule::SetDefaultProcedure(const wxString& procname, const eContentHelper& contentHelper, std::vector<wxString> args)
{
	m_contentHelper.insert_or_assign(procname, CContentData{ contentHelper , args });
}

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

bool CMetaObjectModule::LoadData(CMemoryReader& reader)
{
	//reader.r_stringZ(m_moduleData);
	return m_propertyModule->LoadData(reader);
}

bool CMetaObjectModule::SaveData(CMemoryWriter& writer)
{
	//writer.w_stringZ(m_moduleData);
	return m_propertyModule->SaveData(writer);
}

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CMetaObjectCommonModule::CMetaObjectCommonModule(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObjectModule(name, synonym, comment)
{
}

bool CMetaObjectCommonModule::LoadData(CMemoryReader& reader)
{
	m_propertyModule->LoadData(reader); //reader.r_stringZ(m_moduleData);
	m_propertyGlobalModule->SetValue(reader.r_u8());
	return true;
}

bool CMetaObjectCommonModule::SaveData(CMemoryWriter& writer)
{
	//writer.w_stringZ(m_moduleData);
	m_propertyModule->SaveData(writer);
	writer.w_u8(m_propertyGlobalModule->GetValueAsBoolean());
	return true;
}

bool CMetaObjectCommonModule::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		return CMetaObjectCommonModule::OnAfterCloseMetaObject();
	}

	return IMetaObjectModule::OnPropertyChanging(property, newValue);
}

void CMetaObjectCommonModule::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyGlobalModule == property) {
		CMetaObjectCommonModule::OnBeforeRunMetaObject(newObjectFlag);
	}

	IMetaObjectModule::OnPropertyChanged(property, oldValue, newValue);
}

//***********************************************************************
//*                          common value object                        *
//***********************************************************************

bool CMetaObjectCommonModule::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IMetaObjectModule::OnCreateMetaObject(metaData, flags);
}

bool CMetaObjectCommonModule::OnLoadMetaObject(IMetaData* metaData)
{
	return IMetaObjectModule::OnLoadMetaObject(metaData);
}

bool CMetaObjectCommonModule::OnSaveMetaObject()
{
	return IMetaObjectModule::OnSaveMetaObject();
}

bool CMetaObjectCommonModule::OnDeleteMetaObject()
{
	return IMetaObjectModule::OnDeleteMetaObject();
}

bool CMetaObjectCommonModule::OnRenameMetaObject(const wxString& newName)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RenameCommonModule(this, newName))
		return false;

	return IMetaObjectModule::OnRenameMetaObject(newName);
}

bool CMetaObjectCommonModule::OnBeforeRunMetaObject(int flags)
{
	return IMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectCommonModule::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCommonModule(this, false, (flags & newObjectFlag) != 0))
		return false;

	return IMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectCommonModule::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCommonModule(this))
		return false;

	return IMetaObjectModule::OnAfterCloseMetaObject();
}

bool CMetaObjectCommonModule::OnAfterCloseMetaObject()
{
	return IMetaObjectModule::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                          manager value object                       *
//***********************************************************************

bool CMetaObjectManagerModule::OnBeforeRunMetaObject(int flags)
{
	return IMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectManagerModule::OnAfterRunMetaObject(int flags)
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCommonModule(this, true, (flags & newObjectFlag) != 0))
		return false;

	return IMetaObjectModule::OnAfterRunMetaObject(flags);
}

bool CMetaObjectManagerModule::OnBeforeCloseMetaObject()
{
	IModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCommonModule(this))
		return false;

	return IMetaObjectModule::OnBeforeCloseMetaObject();
}

bool CMetaObjectManagerModule::OnAfterCloseMetaObject()
{
	return IMetaObjectModule::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectModule, "baseModule", g_metaModuleCLSID);

METADATA_TYPE_REGISTER(CMetaObjectCommonModule, "commonModule", g_metaCommonModuleCLSID);
METADATA_TYPE_REGISTER(CMetaObjectManagerModule, "managerModule", g_metaManagerCLSID);
