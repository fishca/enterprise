#ifndef __META_LANGUAGE_H__
#define __META_LANGUAGE_H__

#include "metaObject.h"

class BACKEND_API CMetaObjectLanguage : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectLanguage);
public:

	CMetaObjectLanguage(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	CPropertyName* m_propertyCode = IPropertyObject::CreateProperty<CPropertyName>(m_categorySecondary, wxT("code"), _("Code"), wxEmptyString);
};

#endif 