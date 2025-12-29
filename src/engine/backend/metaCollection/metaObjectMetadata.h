#ifndef __METAOBJECT_METADATA_H__
#define __METAOBJECT_METADATA_H__

#include "metaObject.h"
#include "metaObjectMetadataEnum.h"

#include "metaModuleObject.h"

//*****************************************************************************************
//*                                  metaData object                                      *
//*****************************************************************************************

#define configurationDefaultName _("configuration")

///////////////////////////////////////////////////////////////////////////

class BACKEND_API CMetaObjectConfiguration : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectConfiguration);
private:
	CRole* m_roleAdministration = IMetaObject::CreateRole(wxT("administration"), _("Administration"));
	CRole* m_roleDataAdministration = IMetaObject::CreateRole(wxT("dataAdministration"), _("Data administration"));
	CRole* m_roleUpdateDatabaseConfiguration = IMetaObject::CreateRole(wxT("updateDatabaseConfiguration"), _("Update database configuration"));
	CRole* m_roleActiveUsers = IMetaObject::CreateRole(wxT("activeUsers"), _("Active users"));
	CRole* m_roleExclusiveMode = IMetaObject::CreateRole(wxT("exclusiveMode"), _("Exclusive mode"));
	CRole* m_roleModeAllFunction = IMetaObject::CreateRole(wxT("modeAllFunctions"), _("Mode \"All functions\""));
protected:

	enum
	{
		ID_METATREE_OPEN_INIT_MODULE = 19000,
	};

	CPropertyInnerModule<CMetaObjectModule>* m_propertyModuleConfiguration = IPropertyObject::CreateProperty<CPropertyInnerModule<CMetaObjectModule>>(m_categorySecondary, IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectModule>(wxT("configurationModule"), _("Configuration module")));

	CPropertyCategory* m_compatibilityCategory = IPropertyObject::CreatePropertyCategory(wxT("compatibility"), _("Compatibility"));
	CPropertyEnum<CValueEnumVersion>* m_propertyVersion = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumVersion>>(m_compatibilityCategory, wxT("version"), _("Version"), version_oes_last);

public:

	virtual bool FilterChild(const class_identifier_t& clsid) const {

		if (
			clsid == g_metaCommonModuleCLSID ||
			clsid == g_metaCommonFormCLSID ||
			clsid == g_metaCommonTemplateCLSID ||
			clsid == g_metaRoleCLSID ||
			clsid == g_metaInterfaceCLSID ||
			clsid == g_metaPictureCLSID ||
			clsid == g_metaLanguageCLSID ||
			clsid == g_metaConstantCLSID ||
			clsid == g_metaCatalogCLSID ||
			clsid == g_metaDocumentCLSID ||
			clsid == g_metaEnumerationCLSID ||
			clsid == g_metaDataProcessorCLSID ||
			clsid == g_metaReportCLSID ||
			clsid == g_metaInformationRegisterCLSID ||
			clsid == g_metaAccumulationRegisterCLSID
			)
			return true;

		return false;
	}

	virtual void SetVersion(const version_identifier_t& version) {
		m_propertyVersion->SetValue(static_cast<eProgramVersion>(version));
	}

	version_identifier_t GetVersion() const {
		return m_propertyVersion->GetValueAsInteger();
	}

	CMetaObjectConfiguration();
	virtual ~CMetaObjectConfiguration();

	virtual wxString GetFullName() const {
		return configurationDefaultName;
	}

	virtual wxString GetModuleName() const {
		return configurationDefaultName;
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	//create function 
	static bool ExecuteSystemSQLCommand();

public:

	virtual CMetaObjectModule* GetModuleObject() const { return m_propertyModuleConfiguration->GetMetaObject(); }

protected:

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#endif 