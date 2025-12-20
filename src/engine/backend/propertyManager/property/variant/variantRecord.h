#ifndef __RECORD_VARIANT_H__
#define __RECORD_VARIANT_H__

#include "backend/metaData.h"

class BACKEND_API wxVariantDataRecord : public wxVariantData {
	wxString MakeString() const;
public:

	CMetaDescription& GetMetaDesc() { return m_metaDesc; }
	const CMetaDescription& GetMetaDesc() const { return m_metaDesc; }

	CValue GetDataValue() const;

	wxVariantDataRecord(const IMetaObjectGenericData* prop, const CMetaDescription& typeDesc) : wxVariantData(), m_ownerProperty(prop), m_metaDesc(typeDesc) {}
	wxVariantDataRecord(const wxVariantDataRecord &src) : wxVariantData(), m_ownerProperty(src.m_ownerProperty), m_metaDesc(src.m_metaDesc) {}

	virtual wxVariantDataRecord* Clone() const {
		return new wxVariantDataRecord(*this);
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

	virtual wxString GetType() const { return wxT("wxVariantDataRecord"); }

protected:
	const IMetaObjectGenericData* m_ownerProperty;
	CMetaDescription m_metaDesc;
};

#endif // !_DOCUMENT_VARIANT_
