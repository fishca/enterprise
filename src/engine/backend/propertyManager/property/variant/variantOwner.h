#ifndef __OWNER_VARIANT_H__
#define __OWNER_VARIANT_H__

#include "backend/metaCollection/partial/commonObject.h"

class BACKEND_API ibVariantDataOwner : public wxVariantData {
	wxString MakeString() const;
public:

	ibMetaDescription& GetMetaDesc() { return m_metaDesc; }
	const ibMetaDescription& GetMetaDesc() const { return m_metaDesc; }

	ibValue GetDataValue() const;

	ibVariantDataOwner(const ibValueMetaObjectGenericData* prop, const ibMetaDescription& typeDesc) : wxVariantData(), m_ownerProperty(prop), m_metaDesc(typeDesc) {}
	ibVariantDataOwner(const ibVariantDataOwner& src) : wxVariantData(), m_ownerProperty(src.m_ownerProperty), m_metaDesc(src.m_metaDesc) {}

	virtual ibVariantDataOwner* Clone() const {
		return new ibVariantDataOwner(*this);
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

	virtual wxString GetType() const { return wxT("ibVariantDataOwner"); }

protected:
	const ibValueMetaObjectGenericData* m_ownerProperty;
	ibMetaDescription m_metaDesc;
};

#endif