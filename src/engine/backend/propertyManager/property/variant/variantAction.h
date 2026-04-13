#ifndef __ACTION_VARIANT_H__
#define __ACTION_VARIANT_H__

#include "backend/backend_core.h"
#include "backend/actionInfo.h"

class BACKEND_API wxVariantDataAction : public wxVariantData {
	wxString MakeString() const;
public:

	CActionDescription& GetValueAsActionDesc() { return m_actionData; }

	wxVariantDataAction(const CActionDescription& act) : wxVariantData(), m_actionData(act) {}

	virtual bool Eq(wxVariantData& data) const { 
		wxVariantDataAction* srcData = dynamic_cast<wxVariantDataAction*>(&data);
		if (srcData != nullptr) return m_actionData == srcData->GetValueAsActionDesc();
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

	virtual wxString GetType() const { return wxT("wxVariantDataAction"); }

private:
	CActionDescription m_actionData;
};

#endif