#ifndef __META_LANGUAGE_H__
#define __META_LANGUAGE_H__

#include "metaObject.h"

class BACKEND_API CMetaObjectLanguage : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectLanguage);
public:

	wxString GetCode() const { return m_propertyCode->GetValueAsString(); }
	void SetCode(const wxString& strCode) { m_propertyCode->SetValue(strCode); }

	CMetaObjectLanguage(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//events: 
	virtual bool OnDeleteMetaObject();

	//module manager is started or exit 
	//after and before for designer 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

	/**
	* Property events
	*/
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	bool IsValidCode(const wxString& strLangCode);

	CPropertyName* m_propertyCode =
		IPropertyObject::CreateProperty<CPropertyName>(m_categorySecondary, wxT("code"), _("Code"), wxT("en"));
};

#endif 