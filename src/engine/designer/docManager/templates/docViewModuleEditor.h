#ifndef _MODULE_H__
#define _MODULE_H__

#include "frontend/docView/docView.h"
#include "win/editor/codeEditor/codeEditor.h"

#include <wx/fdrepdlg.h>

// The view using a standard wxTextCtrl to show its contents
class CModuleEditView : public CMetaView {
public:

	CModuleEditView() : CMetaView(), m_codeEditor(nullptr) {}

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnActivateView(bool activate, wxView* activeView, wxView* deactiveView) override;
	virtual void OnDraw(wxDC* dc) override;
	virtual void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	virtual wxPrintout* OnCreatePrintout() override;
	virtual void OnCreateToolbar(wxAuiToolBar* toolbar) override;

	CCodeEditor* GetCodeEditor() const { return m_codeEditor; }

private:

	void OnCopy(wxCommandEvent& WXUNUSED(event)) { m_codeEditor->Copy(); }
	void OnPaste(wxCommandEvent& WXUNUSED(event)) { m_codeEditor->Paste(); }
	void OnSelectAll(wxCommandEvent& WXUNUSED(event)) { m_codeEditor->SelectAll(); }
	void OnFind(wxFindDialogEvent& event) { m_codeEditor->FindText(event.GetFindString(), event.GetFlags()); }

	void OnMenuEvent(wxCommandEvent& event);

protected:

	CCodeEditor* m_codeEditor;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS(CModuleEditView);
};

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

class CModuleDocument : public IValueModuleDocument {
public:

	CModuleDocument() : IValueModuleDocument() {}

	virtual bool OnCreate(const wxString& path, long flags) override;
	virtual bool OnOpenDocument(const wxString& filename) override;
	virtual bool OnSaveDocument(const wxString& filename) override;
	virtual bool OnSaveModified() override;
	virtual bool OnCloseDocument() override;
	virtual wxCommandProcessor* OnCreateCommandProcessor() override;
	
	virtual CCodeEditor* GetCodeEditor() const = 0;

	virtual bool IsModified() const override;
	virtual void Modify(bool mod) override;

	virtual bool Save() override;

protected:

	virtual bool DoSaveDocument(const wxString& filename) override;
	virtual bool DoOpenDocument(const wxString& filename) override;

	wxDECLARE_NO_COPY_CLASS(CModuleDocument);
	wxDECLARE_ABSTRACT_CLASS(CModuleDocument);
};

// ----------------------------------------------------------------------------
// A very simple text document class
// ----------------------------------------------------------------------------

class CModuleEditDocument : public CModuleDocument {
public:
	CModuleEditDocument() : CModuleDocument() {}

	virtual CCodeEditor* GetCodeEditor() const override;

	virtual void SetCurrentLine(int lineBreakpoint, bool setBreakpoint) override;
	virtual void SetToolTip(const wxString& resultStr) override;
	virtual void ShowAutoComplete(const struct CDebugAutoCompleteData& debugData) override;

	wxDECLARE_NO_COPY_CLASS(CModuleEditDocument);
	wxDECLARE_DYNAMIC_CLASS(CModuleEditDocument);
};

#endif