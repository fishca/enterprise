#ifndef __NUMBER_VARIANT_H__
#define __NUMBER_VARIANT_H__

#include "backend/backend_core.h"

class BACKEND_API wxVariantDataNumber : public wxVariantData {
	wxString MakeString() const;
public:

	void SetNumber(const number_t& number) { m_number = number; }
	number_t& GetNumber() { return m_number; }

	wxVariantDataNumber(const number_t& number = 0.0f) : m_number(number) {}

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataNumber* srcData = dynamic_cast<wxVariantDataNumber*>(&data);
		if (srcData != nullptr) {
			return m_number == srcData->GetNumber();
		}
		return false;
	}

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

	virtual wxString GetType() const { return wxT("wxVariantDataNumber"); }

private:
	number_t m_number;
};

#endif