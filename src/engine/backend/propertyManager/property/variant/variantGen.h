#ifndef __GEN_VARIANT_H__
#define __GEN_VARIANT_H__

#include "backend/metaData.h"

class BACKEND_API wxVariantDataGeneration : public wxVariantData {
	wxString MakeString() const;
public:

	CMetaDescription& GetMetaDesc() { return m_metaDesc; }
	const CMetaDescription& GetMetaDesc() const { return m_metaDesc; }

	CValue GetDataValue() const;

	wxVariantDataGeneration(const IValueMetaObjectGenericData* prop, const CMetaDescription& typeDesc) : wxVariantData(), m_ownerProperty(prop), m_metaDesc(typeDesc) {}
	wxVariantDataGeneration(const wxVariantDataGeneration& src) : wxVariantData(), m_ownerProperty(src.m_ownerProperty), m_metaDesc(src.m_metaDesc) {}

	virtual wxVariantDataGeneration* Clone() const {
		return new wxVariantDataGeneration(*this);
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

	virtual wxString GetType() const { return wxT("wxVariantDataGeneration"); }

protected:
	const IValueMetaObjectGenericData* m_ownerProperty;
	CMetaDescription m_metaDesc;
};

#endif