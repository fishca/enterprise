#ifndef __MODULE_VARIANT_H__
#define __MODULE_VARIANT_H__

#include "backend/backend_core.h"

class BACKEND_API wxVariantDataModule : public wxVariantData {
	wxString MakeString() const;
public:

	//set module code 
	void SetModuleText(const wxString& moduleText) { m_moduleData = moduleText; }
	wxString GetModuleText() const { return m_moduleData; }

	wxVariantDataModule(const wxString& moduleData = wxEmptyString) : m_moduleData(moduleData) {}

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataModule* srcData = dynamic_cast<wxVariantDataModule*>(&data);
		if (srcData != nullptr) {
			return m_moduleData == srcData->GetModuleText();
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

	virtual wxString GetType() const { return wxT("wxVariantDataModule"); }

protected:
	wxString m_moduleData;
};

#endif