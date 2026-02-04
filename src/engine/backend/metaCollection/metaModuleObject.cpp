////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metamodule object
////////////////////////////////////////////////////////////////////////////

#include "metaModuleObject.h"
#include "backend/appData.h"

//***********************************************************************
//*                           ModuleObject                              *
//***********************************************************************

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectModule, IValueMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectModule, IValueMetaObjectModule);
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectCommonModule, IValueMetaObjectModule);
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectManagerModule, CValueMetaObjectCommonModule);

//***********************************************************************
//*                           System metaData                           *
//***********************************************************************

#include "backend/debugger/debugClient.h"

bool IValueMetaObjectModule::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IValueMetaObject::OnCreateMetaObject(metaData, flags);
}

bool IValueMetaObjectModule::OnLoadMetaObject(IMetaData* metaData)
{
	return IValueMetaObject::OnLoadMetaObject(metaData);
}

bool IValueMetaObjectModule::OnSaveMetaObject(int flags)
{
	//save debugger client offset
	if ((flags & saveConfigFlag) != 0 && appData->DesignerMode()) {
		const wxString& strBuffer = GetModuleText();
		debugClient->SaveModule(GetDocPath(),
			1 + std::count(strBuffer.begin(), strBuffer.end(), wxT('\n')));
	}

	return IValueMetaObject::OnSaveMetaObject(flags);
}

bool IValueMetaObjectModule::OnDeleteMetaObject()
{
	return IValueMetaObject::OnDeleteMetaObject();
}

bool IValueMetaObjectModule::OnBeforeRunMetaObject(int flags)
{
	//initialize debugger client
	if ((flags & loadConfigFlag) == 0 && (flags & newObjectFlag) == 0 && appData->DesignerMode()) {
		const wxString& strBuffer = GetModuleText();
		debugClient->InitializeModule(GetDocPath(),
			1 + std::count(strBuffer.begin(), strBuffer.end(), wxT('\n')));
	}

	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectModule::OnAfterCloseMetaObject()
{
	//remove debugger client module unit
	//if (appData->DesignerMode())
	//	debugClient->RemoveModule(GetDocPath());

	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                          default procedures						    *
//***********************************************************************

void IValueMetaObjectModule::SetDefaultProcedure(const wxString& procname, const eContentHelper& contentHelper, std::vector<wxString> args)
{
	m_contentHelper.insert_or_assign(procname, CContentData{ contentHelper , args });
}

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

bool CValueMetaObjectModule::LoadData(CMemoryReader& reader)
{
	//reader.r_stringZ(m_moduleData);
	return m_propertyModule->LoadData(reader);
}

bool CValueMetaObjectModule::SaveData(CMemoryWriter& writer)
{
	//writer.w_stringZ(m_moduleData);
	return m_propertyModule->SaveData(writer);
}

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CValueMetaObjectCommonModule::CValueMetaObjectCommonModule(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObjectModule(name, synonym, comment)
{
}

bool CValueMetaObjectCommonModule::LoadData(CMemoryReader& reader)
{
	m_propertyModule->LoadData(reader); //reader.r_stringZ(m_moduleData);
	m_propertyGlobalModule->SetValue(reader.r_u8());
	return true;
}

bool CValueMetaObjectCommonModule::SaveData(CMemoryWriter& writer)
{
	//writer.w_stringZ(m_moduleData);
	m_propertyModule->SaveData(writer);
	writer.w_u8(m_propertyGlobalModule->GetValueAsBoolean());
	return true;
}

//***********************************************************************
//*                          common value object                        *
//***********************************************************************

#include "backend/metaData.h"

bool CValueMetaObjectCommonModule::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IValueMetaObjectModule::OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectCommonModule::OnLoadMetaObject(IMetaData* metaData)
{
	return IValueMetaObjectModule::OnLoadMetaObject(metaData);
}

bool CValueMetaObjectCommonModule::OnSaveMetaObject(int flags)
{
	return IValueMetaObjectModule::OnSaveMetaObject(flags);
}

bool CValueMetaObjectCommonModule::OnDeleteMetaObject()
{
	return IValueMetaObjectModule::OnDeleteMetaObject();
}

bool CValueMetaObjectCommonModule::OnRenameMetaObject(const wxString& newName)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RenameCommonModule(this, newName))
		return false;

	return IValueMetaObjectModule::OnRenameMetaObject(newName);
}

bool CValueMetaObjectCommonModule::OnBeforeRunMetaObject(int flags)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCommonModule(this, false, (flags & newObjectFlag) != 0))
		return false;

	return IValueMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectCommonModule::OnAfterRunMetaObject(int flags)
{
	return IValueMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectCommonModule::OnBeforeCloseMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCommonModule(this))
		return false;

	return IValueMetaObjectModule::OnAfterCloseMetaObject();
}

bool CValueMetaObjectCommonModule::OnAfterCloseMetaObject()
{
	return IValueMetaObjectModule::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                          manager value object                       *
//***********************************************************************

bool CValueMetaObjectManagerModule::OnBeforeRunMetaObject(int flags)
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->AddCommonModule(this, true, (flags & newObjectFlag) != 0))
		return false;

	return IValueMetaObjectModule::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectManagerModule::OnAfterRunMetaObject(int flags)
{
	return IValueMetaObjectModule::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectManagerModule::OnBeforeCloseMetaObject()
{
	IValueModuleManager* moduleManager = m_metaData->GetModuleManager();
	wxASSERT(moduleManager);

	if (!moduleManager->RemoveCommonModule(this))
		return false;

	return IValueMetaObjectModule::OnBeforeCloseMetaObject();
}

bool CValueMetaObjectManagerModule::OnAfterCloseMetaObject()
{
	return IValueMetaObjectModule::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectModule, "module", g_metaModuleCLSID);

METADATA_TYPE_REGISTER(CValueMetaObjectCommonModule, "commonModule", g_metaCommonModuleCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectManagerModule, "managerModule", g_metaManagerCLSID);
