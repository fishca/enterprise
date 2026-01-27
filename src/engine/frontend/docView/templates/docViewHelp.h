#ifndef __HELP_H__
#define __HELP_H__

// ----------------------------------------------------------------------------
// Edit form classes
// ----------------------------------------------------------------------------

#include "frontend/docView/docView.h"
#include "frontend/win/editor/textEditor/textEditor.h"

#include <wx/fdrepdlg.h>

// The view using a standard wxTextCtrl to show its contents
class FRONTEND_API CHelpEditView : public CMetaView {
public:
	
	CHelpEditView() : CMetaView(), m_textEditor(nullptr) {}

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnDraw(wxDC* dc) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	virtual wxPrintout* OnCreatePrintout() override;

	CTextEditor* GetText() const { return m_textEditor; }

private:

	void OnCopy(wxCommandEvent& WXUNUSED(event)) { m_textEditor->Copy(); }
	void OnPaste(wxCommandEvent& WXUNUSED(event)) { m_textEditor->Paste(); }
	void OnSelectAll(wxCommandEvent& WXUNUSED(event)) { m_textEditor->SelectAll(); }

	void OnFind(wxFindDialogEvent& event);

	CTextEditor* m_textEditor;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS(CHelpEditView);
};

// ----------------------------------------------------------------------------
// IHelpDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

#include "backend/metaCollection/metaModuleObject.h"

class FRONTEND_API IHelpDocument : public CMetaDocument
{
public:

	virtual wxIcon GetIcon() const {
		return CBackendPicture::GetPictureAsIcon(g_metaCommonModuleCLSID);
	}

	IHelpDocument() : CMetaDocument() { m_childDoc = false; }

	virtual wxCommandProcessor* OnCreateCommandProcessor() override;
	virtual CTextEditor* GetTextCtrl() const;

protected:
	wxDECLARE_NO_COPY_CLASS(IHelpDocument);
	wxDECLARE_ABSTRACT_CLASS(IHelpDocument);
};

// ----------------------------------------------------------------------------
// A very simple text document class
// ----------------------------------------------------------------------------

class FRONTEND_API CHelpFileDocument : public IHelpDocument
{
public:
	
	CHelpFileDocument() : IHelpDocument(), m_loadFromFile(false) {}

	virtual bool OnCreate(const wxString& path, long flags) override;
	virtual bool OnNewDocument() override {

		// notice that there is no need to either reset nor even check the
		// modified flag here as the document itself is a new object (this is only
		// called from CreateDocument()) and so it shouldn't be saved anyhow even
		// if it is modified -- this could happen if the user code creates
		// documents pre-filled with some user-entered (and which hence must not be
		// lost) information

		SetDocumentSaved(false);

		const wxString name = GetDocumentManager()->MakeNewDocumentName();
	
		SetTitle(name);
		SetFilename(name, true);
		Modify(true);

		return true;
	}

protected:

	virtual bool DoSaveDocument(const wxString& filename) override;
	virtual bool DoOpenDocument(const wxString& filename) override;

	bool m_loadFromFile;

	wxDECLARE_NO_COPY_CLASS(CHelpFileDocument);
	wxDECLARE_DYNAMIC_CLASS(CHelpFileDocument);
};

#endif