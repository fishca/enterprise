#ifndef __META_PICTURE_H__
#define __META_PICTURE_H__

#include "metaObject.h"

class BACKEND_API CMetaObjectPicture : public IMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CMetaObjectPicture);
public:

	wxBitmap GetValueAsBitmap() const { return m_propertyPicture->GetValueAsBitmap(); }

	CMetaObjectPicture(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	CPropertyExternalPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyExternalPicture>(m_categorySecondary, wxT("picture"), _("Picture"));
};

#endif 