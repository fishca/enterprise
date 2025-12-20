#ifndef __OWNER_VARIANT_H__
#define __OWNER_VARIANT_H__

#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API wxVariantDataOwner : public wxVariantData {
	wxString MakeString() const;
public:

	CMetaDescription& GetMetaDesc() { return m_metaDesc; }
	const CMetaDescription& GetMetaDesc() const { return m_metaDesc; }

	CValue GetDataValue() const;

	wxVariantDataOwner(const IMetaObjectGenericData* prop, const CMetaDescription& typeDesc) : wxVariantData(), m_ownerProperty(prop), m_metaDesc(typeDesc) {}
	wxVariantDataOwner(const wxVariantDataOwner& src) : wxVariantData(), m_ownerProperty(src.m_ownerProperty), m_metaDesc(src.m_metaDesc) {}

	virtual wxVariantDataOwner* Clone() const {
		return new wxVariantDataOwner(*this);
	}

	bool Eq(wxVariantData& data) const { return true; }

#if wxUSE_STD_IOSTREAM
	virtual bool Write(wxSTD ostream& str) const {
		str << MakeString();
		return true;
	}
#endif
	virtual bool Write(wxString& str) const {
		str = MakeString();
		return true;
	}

	virtual wxString GetType() const { return wxT("wxVariantDataOwner"); }

protected:
	const IMetaObjectGenericData* m_ownerProperty;
	CMetaDescription m_metaDesc;
};

#endif