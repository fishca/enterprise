/////////////////////////////////////////////////////////////////////////////
// Name:        datavgen.paged.private.h
// Purpose:     Private helpers shared between datavgen.cpp and
//              datavgen.paged.cpp — the paged-bootstrap split (2026-05-08)
//              moved several methods out of the main translation unit but
//              left their dependencies (RAII freeze guard + buffer-slack
//              constant) in the original file.  This header re-exposes
//              them at namespace scope so both .cpp files compile.
//
//              Header is intended for these two .cpp files only.  Do not
//              include from public headers or other translation units.
/////////////////////////////////////////////////////////////////////////////

#ifndef OES_DATAVGEN_PAGED_PRIVATE_H
#define OES_DATAVGEN_PAGED_PRIVATE_H

#include "dataview.h"
#ifdef __WXMSW__
#include <wx/app.h>
#endif

// ibDataViewMainWindow — the inner table-area window owned by
// ibDataViewCtrl.  Forward-declared in datavgen.h (so the public
// surface stays small) and defined here so paged code paths in
// datavgen.paged.cpp can call its methods.  Definition keeps to its
// original .cpp-local layout — wxDECLARE_DYNAMIC_CLASS / EVENT_TABLE
// implementations stay in datavgen.cpp (out-of-line).
class ibDataViewMainWindow : public wxWindow
{
public:

	// table window variants for scrolling possibilities
	enum ibDataViewWindowType
	{
		ibDataViewWindowNormal = 0,
		ibDataViewWindowFrozenCol = 1,
		ibDataViewWindowFrozenRow = 2,
		ibDataViewWindowFrozenCorner = ibDataViewWindowFrozenCol | ibDataViewWindowFrozenRow
	};

	ibDataViewMainWindow(ibDataViewCtrl* owner,
		ibDataViewWindowType type, int additionalStyle = wxWANTS_CHARS | wxCLIP_CHILDREN,
		const wxString& name = wxT("wxdataviewctrlmainwindow"))
		: m_owner(NULL), m_type(type)
	{
		// We want to use a specific class name for this window in wxMSW to make it
		// possible to configure screen readers to handle it specifically.
#ifdef __WXMSW__
		CreateUsingMSWClass
		(
			wxApp::GetRegisteredClassName
			(
				wxT("ibDataView"),
				-1, // no specific background brush
				0, // no special styles either
				wxApp::RegClass_OnlyNR
			),
			owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, additionalStyle | wxBORDER_NONE, name
		);
#else
		Create(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_NONE, name);
#endif

		SetOwner(owner);

		SetBackgroundColour(*wxWHITE);

		SetBackgroundStyle(wxBG_STYLE_PAINT);
	}

	void SetOwner(ibDataViewCtrl* owner) { m_owner = owner; }
	ibDataViewCtrl* GetOwner() { return m_owner; }
	const ibDataViewCtrl* GetOwner() const { return m_owner; }

	ibDataViewModel* GetModel() { return GetOwner()->GetModel(); }
	const ibDataViewModel* GetModel() const { return GetOwner()->GetModel(); }

	virtual wxWindow* GetMainWindowOfCompositeControl() wxOVERRIDE
	{
		return GetOwner();
	}

	virtual void ScrollWindow(int dx, int dy, const wxRect* rect = NULL) wxOVERRIDE;

	ibDataViewWindowType GetType() const { return m_type; }

protected:

	void OnPaint(wxPaintEvent& event);
	void OnCharHook(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnSetFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);

private:

	ibDataViewCtrl* m_owner;

	const ibDataViewWindowType m_type;

	wxDECLARE_DYNAMIC_CLASS(ibDataViewCtrl);
	wxDECLARE_EVENT_TABLE();
};

// Slack rows kept on each side of the viewport.  Buffer target =
// viewport + 2*kBufferSlack, so a +/-kBufferSlack scroll within the
// loaded window does not trigger any fetch.  Once the visible margin
// to either edge drops below this, fetch the next batch and trim the
// far side back to target.
static constexpr long kBufferSlack = 5;

namespace {
// RAII pair-Freeze for ibDataViewCtrl + its main table window.  Most
// paged-mode operations need both windows frozen across a buffer
// mutation + scroll restore (otherwise wxScrollHelper's pixel<->row
// mapping flickers between intermediate states).  Auto-Thaws on scope
// exit — early returns are safe.
class ScopedPagedFreeze {
public:
	explicit ScopedPagedFreeze(ibDataViewCtrl* ctrl)
		: m_ctrl(ctrl), m_table(ctrl ? ctrl->GetMainWindow() : nullptr)
	{
		if (m_ctrl)  m_ctrl->Freeze();
		if (m_table) m_table->Freeze();
	}
	~ScopedPagedFreeze() {
		if (m_table) m_table->Thaw();
		if (m_ctrl)  m_ctrl->Thaw();
	}
	ScopedPagedFreeze(const ScopedPagedFreeze&) = delete;
	ScopedPagedFreeze& operator=(const ScopedPagedFreeze&) = delete;
private:
	ibDataViewCtrl* m_ctrl;
	wxWindow*       m_table;
};
} // anonymous namespace

#endif // OES_DATAVGEN_PAGED_PRIVATE_H
