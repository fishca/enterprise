#ifndef __GEN_VARIANT_H__
#define __GEN_VARIANT_H__

#include "backend/metaData.h"

class BACKEND_API ibVariantDataGeneration : public wxVariantData {
	wxString MakeString() const;
public:

	ibMetaDescription& GetMetaDesc() { return m_metaDesc; }
	const ibMetaDescription& GetMetaDesc() const { return m_metaDesc; }

	ibValue GetDataValue() const;

	ibVariantDataGeneration(const ibValueMetaObjectGenericData* prop, const ibMetaDescription& typeDesc) : wxVariantData(), m_ownerProperty(prop), m_metaDesc(typeDesc) {}
	ibVariantDataGeneration(const ibVariantDataGeneration& src) : wxVariantData(), m_ownerProperty(src.m_ownerProperty), m_metaDesc(src.m_metaDesc) {}

	virtual ibVariantDataGeneration* Clone() const {
		return new ibVariantDataGeneration(*this);
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

	virtual wxString GetType() const { return wxT("ibVariantDataGeneration"); }

protected:
	const ibValueMetaObjectGenericData* m_ownerProperty;
	ibMetaDescription m_metaDesc;
};

#endif