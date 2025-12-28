#ifndef __META_LANGUAGE_H__
#define __META_LANGUAGE_H__

#include "metaObject.h"

class BACKEND_API CMetaObjectLanguage : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectLanguage);
public:

	wxString GetLanguageCode() const {
		return m_propertyCode->GetValueAsString();
	}

	CMetaObjectLanguage(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	bool IsValidCode(const wxString& code);

	CPropertyName* m_propertyCode = 
		IPropertyObject::CreateProperty<CPropertyName>(m_categorySecondary, wxT("code"), _("Code"), wxT("en"));
};

#endif 