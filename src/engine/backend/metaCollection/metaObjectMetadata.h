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

class BACKEND_API CValueMetaObjectConfiguration : public IValueMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectConfiguration);

	enum
	{
		ID_METATREE_OPEN_INIT_MODULE = 19000,
	};

public:

#pragma region access
	bool AccessRight_Administration(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleAdministration, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
	bool AccessRight_DataAdministration(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleDataAdministration, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
	bool AccessRight_UpdateDatabaseConfiguration(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleUpdateDatabaseConfiguration, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
	bool AccessRight_ActiveUsers(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleActiveUsers, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
	bool AccessRight_ExclusiveMode(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleExclusiveMode, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
	bool AccessRight_ModeAllFunction(const CUserRoleInfo& roleInfo = CUserRoleInfo()) const { return AccessRight(m_roleModeAllFunction, roleInfo.IsSetRole() ? roleInfo : GetUserRoleInfo()); }
#pragma endregion

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

	void SetVersion(const version_identifier_t& version) { m_propertyVersion->SetValue(static_cast<eProgramVersion>(version)); }
	version_identifier_t GetVersion() const { return m_propertyVersion->GetValueAsInteger(); }

	void SetLanguage(const meta_identifier_t& id) { m_propertyDefLanguage->SetValue(id); }
	meta_identifier_t GetLanguage() const { return m_propertyDefLanguage->GetValueAsInteger(); }

	//////////////////////////////////////////////////////////////////////////////////////////////

	wxString GetLangCode() const;

	//////////////////////////////////////////////////////////////////////////////////////////////

	CValueMetaObjectConfiguration();
	virtual ~CValueMetaObjectConfiguration();

	virtual wxString GetFullName() const { return configurationDefaultName; }
	virtual wxString GetModuleName() const { return configurationDefaultName; }

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

	/**
	* Property events
	*/
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	//create function 
	static bool ExecuteSystemSQLCommand();

public:

	virtual CValueMetaObjectModule* GetModuleObject() const { return m_propertyModuleConfiguration->GetMetaObject(); }

protected:

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	bool FillRoleList(CPropertyList* prop) {
		std::vector<IValueMetaObject*> array;
		if (FillArrayObjectByFilter(array, { g_metaRoleCLSID })) {
			for (const auto child : array) {
				prop->AppendItem(
					child->GetName(),
					child->GetMetaID(),
					child->GetIcon(),
					child);
			}
			return true;
		}
		return false;
	}

	bool FillLanguageList(CPropertyList* prop) {
		std::vector<IValueMetaObject*> array;
		if (FillArrayObjectByFilter(array, { g_metaLanguageCLSID })) {
			for (const auto child : array) {
				prop->AppendItem(
					child->GetName(),
					child->GetMetaID(),
					child->GetIcon(),
					child);
			}
			return true;
		}
		return false;
	}

	CPropertyInnerModule<CValueMetaObjectModule>* m_propertyModuleConfiguration = IPropertyObject::CreateProperty<CPropertyInnerModule<CValueMetaObjectModule>>(m_categorySecondary, IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectModule>(wxT("configurationModule"), _("Configuration module")));

	CPropertyCategory* m_propertyPresetValues = IPropertyObject::CreatePropertyCategory(wxT("presetValues"), _("Preset values"));
	CPropertyList* m_propertyDefRole = IPropertyObject::CreateProperty<CPropertyList>(m_propertyPresetValues, wxT("defaultRole"), _("Default role"), _("Default configuration role"), &CValueMetaObjectConfiguration::FillRoleList);
	CPropertyList* m_propertyDefLanguage = IPropertyObject::CreateProperty<CPropertyList>(m_propertyPresetValues, wxT("defaultLanguage"), _("Default language"), _("Default configuration language"), &CValueMetaObjectConfiguration::FillLanguageList);

	CPropertyCategory* m_compatibilityCategory = IPropertyObject::CreatePropertyCategory(wxT("compatibility"), _("Compatibility"));
	CPropertyEnum<CValueEnumVersion>* m_propertyVersion = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumVersion>>(m_compatibilityCategory, wxT("version"), _("Version"), version_oes_last);

#pragma region role 
	CRole* m_roleAdministration = IValueMetaObject::CreateRole(wxT("administration"), _("Administration"));
	CRole* m_roleDataAdministration = IValueMetaObject::CreateRole(wxT("dataAdministration"), _("Data administration"));
	CRole* m_roleUpdateDatabaseConfiguration = IValueMetaObject::CreateRole(wxT("updateDatabaseConfiguration"), _("Update database configuration"));
	CRole* m_roleActiveUsers = IValueMetaObject::CreateRole(wxT("activeUsers"), _("Active users"));
	CRole* m_roleExclusiveMode = IValueMetaObject::CreateRole(wxT("exclusiveMode"), _("Exclusive mode"));
	CRole* m_roleModeAllFunction = IValueMetaObject::CreateRole(wxT("modeAllFunctions"), _("Mode \"All functions\""));
#pragma endregion
};

#endif 