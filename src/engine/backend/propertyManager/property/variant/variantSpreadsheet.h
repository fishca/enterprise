#ifndef __TEMPLATE_VARIANT_H__
#define __TEMPLATE_VARIANT_H__

#include "backend/backend_spreadsheet.h"

class BACKEND_API wxVariantDataSpreadsheet : public wxVariantData {
	wxString MakeString() const;
public:

	bool IsEmptySpreadsheet() const { return m_spreadsheetDesc.IsEmptySpreadsheet(); }

	CSpreadsheetDescription& GetSpreadsheetDesc() { return m_spreadsheetDesc; }

	wxVariantDataSpreadsheet(const CSpreadsheetDescription& val) : wxVariantData(), m_spreadsheetDesc(val) {}

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataSpreadsheet* srcData = dynamic_cast<wxVariantDataSpreadsheet*>(&data);
		if (srcData != nullptr)
			return srcData->m_spreadsheetDesc == m_spreadsheetDesc;
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

	virtual wxString GetType() const { return wxT("wxVariantDataSpreadsheet"); }

private:
	CSpreadsheetDescription m_spreadsheetDesc;
};

#endif