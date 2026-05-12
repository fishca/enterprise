#ifndef __ACTION_VARIANT_H__
#define __ACTION_VARIANT_H__

#include "backend/backend_core.h"
#include "backend/actionInfo.h"

class BACKEND_API ibVariantDataAction : public wxVariantData {
	wxString MakeString() const;
public:

	ibActionDescription& GetValueAsActionDesc() { return m_actionData; }

	ibVariantDataAction(const ibActionDescription& act) : wxVariantData(), m_actionData(act) {}

	virtual bool Eq(wxVariantData& data) const { 
		ibVariantDataAction* srcData = dynamic_cast<ibVariantDataAction*>(&data);
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

	virtual wxString GetType() const { return wxT("ibVariantDataAction"); }

private:
	ibActionDescription m_actionData;
};

#endif