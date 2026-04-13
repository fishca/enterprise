#ifndef _DYNAMIC_BORDER_H__
#define _DYNAMIC_BORDER_H__

#include <wx/dcscreen.h>
#include <wx/stattext.h>

#include "frontend/frontend.h"

class FRONTEND_API wxDynamicStaticText : public wxStaticText {

	mutable struct {

		wxString m_strLabel;
		wxFont m_fontLabel;
		wxSize m_sizeLabel = { 0, 0 };

		inline bool IsSameAs(const wxString& label, const wxFont& font) const {
			return m_strLabel.IsSameAs(label) && m_fontLabel.IsSameAs(font);
		}

	} m_staticTextCache;

	static wxScreenDC ms_calcLabelDC;

public:

	wxDynamicStaticText(wxWindow* parent,
		wxWindowID id, const wxString& label, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxST_ELLIPSIZE_MASK, const wxString& name = wxASCII_STR(wxStaticTextNameStr))
	{
		wxStaticText::Create(parent, id, label, pos, size, style | wxST_NO_AUTORESIZE, name);
	}

	virtual void SetLabel(const wxString& label) override {
		wxStaticText::SetLabel(label);
	}

protected:

	virtual wxSize DoGetBestClientSize() const override;
};

class FRONTEND_API wxControlDynamicBorder {
public:

	virtual bool AllowCalc() const { return true; }

	virtual wxWindow* GetControl() const = 0;
	virtual wxSize GetControlSize() const = 0;

	virtual void CalculateLabelSize(wxCoord* w, wxCoord* h) const = 0;
	virtual void ApplyLabelSize(const wxSize& s) = 0;
};

#endif 