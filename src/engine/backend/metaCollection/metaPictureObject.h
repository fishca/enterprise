#ifndef __META_PICTURE_H__
#define __META_PICTURE_H__

#include "metaObject.h"

class BACKEND_API CValueMetaObjectPicture : public IValueMetaObject {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectPicture);
public:

	wxBitmap GetValueAsBitmap() const { return m_propertyPicture->GetValueAsBitmap(); }

	CValueMetaObjectPicture(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString);

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyExternalPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyExternalPicture>(m_categorySecondary, wxT("Picture"), _("Picture"));
};

#endif 