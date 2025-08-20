#ifndef _CHECK_BOX_H__
#define _CHECK_BOX_H__

#include <wx/checkbox.h>
#include <wx/compositewin.h>
#include <wx/containr.h>

#include "dynamicBorder.h"

#include "frontend/frontend.h"

class FRONTEND_API wxControlCheckboxCtrl :

	public wxCompositeWindow<wxNavigationEnabled<wxWindow>>,
	public wxControlDynamicBorder {

	class wxControlStaticTextCtrl : public wxDynamicStaticText {
	public:

		wxControlStaticTextCtrl(wxWindow* parent,
			wxWindowID id, const wxString& label, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxST_ELLIPSIZE_MASK, const wxString& name = wxASCII_STR(wxStaticTextNameStr)) :
			wxDynamicStaticText(parent, id, label, pos, size, style, name)
		{
			// Ensure that our best size is recomputed using our overridden
			// DoGetBestSize().
			InvalidateBestSize();
		}
	};

	wxDynamicStaticText* m_label;
	wxCheckBox* m_checkBox;
	wxAlignment m_align;

public:

	wxControlCheckboxCtrl() :
		m_label(nullptr), m_checkBox(nullptr), m_align(wxAlignment::wxALIGN_LEFT)
	{
	}

	wxControlCheckboxCtrl(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE) :
		m_label(nullptr), m_checkBox(nullptr)
	{
		Create(parent, id, pos, size, style);
	}

	virtual ~wxControlCheckboxCtrl() {
		delete m_label;
		delete m_checkBox;
	}

	bool Create(wxWindow* parent = nullptr,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0)
	{
		if (!wxCompositeWindow::Create(parent, id, pos, size, style))
			return false;

		m_label = new wxControlStaticTextCtrl(this, wxID_ANY, wxEmptyString);
		m_checkBox = new wxCheckBox(this,
			wxID_ANY,
			wxEmptyString,
			wxDefaultPosition,
			wxDefaultSize,
			style
		);

		return true;
	}

	virtual void SetWindowStyleFlag(long style) override {

		if ((style & wxAlignment::wxALIGN_LEFT) != 0 || style == wxAlignment::wxALIGN_LEFT) {
			m_align = wxAlignment::wxALIGN_LEFT;
		}
		else if ((style & wxAlignment::wxALIGN_RIGHT || style == wxAlignment::wxALIGN_RIGHT) != 0) {
			m_align = wxAlignment::wxALIGN_RIGHT;
		}

		wxWindow::SetWindowStyleFlag(style);
	}

	virtual void SetLabel(const wxString& label) { m_label->SetLabel(label); }
	virtual wxString GetLabel() const { return m_label->GetLabel(); }
	virtual void SetValue(bool value) { m_checkBox->SetValue(value); }
	virtual bool GetValue() const { return m_checkBox->GetValue(); }

	void LayoutControls() {

		if (!m_checkBox)
			return;

		const wxSize sizeTotal = GetBestSize();
		int width = sizeTotal.x,
			height = sizeTotal.y;

		wxSize sizeLabel = m_label->GetBestSize();
		wxSize sizeCheckBox = m_checkBox->GetBestSize();

		if (m_align == wxAlignment::wxALIGN_LEFT) {
			m_label->SetSize(0, (height - sizeLabel.y) / 2, sizeLabel.x, height);
			m_checkBox->SetSize(sizeLabel.x + 1, 0, width - sizeLabel.x - 1, height);
		}
		else {
			m_label->SetSize(sizeCheckBox.x, (height - sizeLabel.y) / 2, sizeLabel.x, height);
			m_checkBox->SetSize(0, 0, sizeCheckBox.x, height);
		}
	}

	// overridden base class virtuals
	virtual bool SetBackgroundColour(const wxColour& colour) {
		m_label->SetBackgroundColour(colour);
		m_checkBox->SetBackgroundColour(colour);
		return wxCompositeWindow::SetBackgroundColour(colour);
	}

	virtual bool SetForegroundColour(const wxColour& colour) {
		m_label->SetForegroundColour(colour);
		m_checkBox->SetForegroundColour(colour);
		return wxCompositeWindow::SetForegroundColour(colour);
	}

	virtual bool SetFont(const wxFont& font) {
		m_label->SetFont(font);
		m_checkBox->SetFont(font);
		return wxCompositeWindow::SetFont(font);
	}

	virtual bool Enable(bool enable = true) {
		return m_checkBox->Enable(enable);
	}

	virtual bool AllowCalc() const {
		return m_align == wxAlignment::wxALIGN_LEFT;
	};

	//dynamic border 
	virtual wxDynamicStaticText* GetStaticText() const { return m_label; }
	virtual wxWindow* GetControl() const { return m_checkBox; }
	virtual wxSize GetControlSize() const { return m_checkBox->GetSize(); }

	virtual void CalculateLabelSize(wxCoord* w, wxCoord* h) const;
	virtual void ApplyLabelSize(const wxSize& s);

protected:

#if wxUSE_TOOLTIPS
	virtual void DoSetToolTipText(const wxString& tip) override {
		m_label->SetToolTip(tip);
		m_checkBox->SetToolTip(tip);
	}

	virtual void DoSetToolTip(wxToolTip* tip) override {
		m_label->SetToolTip(tip);
		m_checkBox->SetToolTip(tip);
	}
#endif // wxUSE_TOOLTIPS

	// override the base class virtuals involved into geometry calculations
	virtual wxSize DoGetBestClientSize() const override;

	void OnSize(wxSizeEvent& event) { LayoutControls(); event.Skip(); }
	void OnDPIChanged(wxDPIChangedEvent& event) { LayoutControls(); event.Skip(); }

private:

	// Implement pure virtual function inherited from wxCompositeWindow.
	virtual wxWindowList GetCompositeWindowParts() const override;

	wxDECLARE_DYNAMIC_CLASS(wxControlCheckboxCtrl);
	wxDECLARE_NO_COPY_CLASS(wxControlCheckboxCtrl);
	wxDECLARE_EVENT_TABLE();
};

#endif 