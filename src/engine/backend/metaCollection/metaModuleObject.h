#ifndef _METAMODULE_OBJECT_H__
#define _METAMODULE_OBJECT_H__

#include "metaObject.h"

enum eContentHelper {
	eProcedureHelper = 1,
	eFunctionHelper,

	eUnknownHelper = 100
};

#pragma region _property_
//base property for "inner module"
template <typename T>
class CPropertyInnerModule : public IProperty {
public:

	T* GetMetaObject() const { return m_metaObject; }

	CPropertyInnerModule(CPropertyCategory* cat, T* metaObject)
		: IProperty(cat, metaObject->GetName(), metaObject->GetSynonym(), wxNullVariant), m_metaObject(metaObject)
	{
	}

	virtual ~CPropertyInnerModule() {}

	// get meta object 
	T* operator->() { return GetMetaObject(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGHyperLinkProperty(m_metaObject, m_propLabel, m_propName, m_propValue);
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) { return false; }
	virtual bool GetDataValue(CValue& pvarPropVal) const {
		pvarPropVal = m_metaObject;
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) { return m_metaObject->GetModuleProperty()->LoadData(reader); }
	virtual bool SaveData(CMemoryWriter& writer) { return m_metaObject->GetModuleProperty()->SaveData(writer); }

private:
	CValuePtr<T> m_metaObject;
};

#pragma endregion

class BACKEND_API IMetaObjectModule : public IMetaObject {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectModule);
public:

	IMetaObjectModule(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString)
		: IMetaObject(name, synonym, comment) {
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events:
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	//get property
	virtual IProperty* GetModuleProperty() const = 0;

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) = 0;
	virtual wxString GetModuleText() const = 0;

	//set default procedures 
	void SetDefaultProcedure(const wxString& procName, const eContentHelper& contentHelper, std::vector<wxString> args = {});

	size_t GetDefaultProcedureCount() const {
		return m_contentHelper.size();
	}

	wxString GetDefaultProcedureName(size_t idx) const {
		if (idx > m_contentHelper.size())
			return wxEmptyString;

		auto it = m_contentHelper.begin();
		std::advance(it, idx);
		return it->first;
	}

	eContentHelper GetDefaultProcedureType(size_t idx) const {
		if (idx > m_contentHelper.size())
			return eContentHelper::eUnknownHelper;
		auto it = m_contentHelper.begin();
		std::advance(it, idx);
		return it->second.m_contentType;
	}

	std::vector<wxString> GetDefaultProcedureArgs(size_t idx) const {
		if (idx > m_contentHelper.size())
			return std::vector<wxString>();
		auto it = m_contentHelper.begin();
		std::advance(it, idx);
		return it->second.m_args;
	}

	virtual bool IsGlobalModule() const { return false; }

private:

	struct CContentData {
		eContentHelper m_contentType;
		std::vector<wxString> m_args;
	};

	std::map<wxString, CContentData> m_contentHelper;
};

class BACKEND_API CMetaObjectModule : public IMetaObjectModule {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectModule);
public:
	CMetaObjectModule(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString)
		: IMetaObjectModule(name, synonym, comment)
	{
	}

	//get property
	virtual IProperty* GetModuleProperty() const { return m_propertyModule; }

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) { m_propertyModule->SetValue(moduleText); }
	virtual wxString GetModuleText() const { return m_propertyModule->GetValueAsString(); }

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyModule* m_propertyModule = IPropertyObject::CreateProperty<CPropertyModule>(m_categorySecondary, wxT("module"), _("Module"));
};

class BACKEND_API CMetaObjectCommonModule : public IMetaObjectModule {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectCommonModule);
private:
	enum
	{
		ID_METATREE_OPEN_MODULE = 19000,
	};

public:

	CMetaObjectCommonModule(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events:
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags);
	virtual bool OnDeleteMetaObject();

	virtual bool OnRenameMetaObject(const wxString& sNewName);

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

	//get property
	virtual IProperty* GetModuleProperty() const { return m_propertyModule; }

	//set module code 
	virtual void SetModuleText(const wxString& moduleText) { m_propertyModule->SetValue(moduleText); }
	virtual wxString GetModuleText() const { return m_propertyModule->GetValueAsString(); }

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);

	// check gm
	virtual bool IsGlobalModule() const {
		return m_propertyGlobalModule->GetValueAsBoolean();
	}

	/**
	* Property events
	*/
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyModule* m_propertyModule = IPropertyObject::CreateProperty<CPropertyModule>(m_categorySecondary, wxT("module"), _("Module"));
	CPropertyCategory* m_moduleCategory = IPropertyObject::CreatePropertyCategory(wxT("common module"), _("Common module"));
	CPropertyBoolean* m_propertyGlobalModule = IPropertyObject::CreateProperty<CPropertyBoolean>(m_moduleCategory, wxT("globalModule"), _("Global module"), false);
};

class BACKEND_API CMetaObjectManagerModule : public CMetaObjectCommonModule {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectManagerModule);
public:
	CMetaObjectManagerModule(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString)
		: CMetaObjectCommonModule(name, synonym, comment)
	{
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	virtual bool OnBeforeCloseMetaObject();
	virtual bool OnAfterCloseMetaObject();

private:
	CPropertyModule* m_propertyModule = IPropertyObject::CreateProperty<CPropertyModule>(m_categorySecondary, wxT("module"), _("Module"));
};

#endif