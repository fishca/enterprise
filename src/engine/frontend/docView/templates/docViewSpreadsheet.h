#ifndef __GRID_H__
#define __GRID_H__

#include "frontend/docView/docView.h"
#include "frontend/win/editor/gridEditor/gridEditor.h"

// ----------------------------------------------------------------------------
// Edit form classes
// ----------------------------------------------------------------------------

// The view using a standard wxTextCtrl to show its contents
class FRONTEND_API CSpreadsheetEditView : public CMetaView
{
public:
	CSpreadsheetEditView() : CMetaView(), m_gridEditor(nullptr) {}

#if wxUSE_MENUS	
	wxMenuBar* CSpreadsheetEditView::CreateMenuBar() const;
#endif 

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnActivateView(bool activate, wxView* activeView, wxView* deactiveView) override;
	virtual void OnDraw(wxDC* dc) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	CGridEditor* GetGridCtrl() const { return m_gridEditor; }

private:

	void OnCopy(wxCommandEvent& WXUNUSED(event)) { m_gridEditor->Copy(); }
	void OnPaste(wxCommandEvent& WXUNUSED(event)) { m_gridEditor->Paste(); }
	void OnSelectAll(wxCommandEvent& WXUNUSED(event)) { m_gridEditor->SelectAll(); }

	virtual wxPrintout* OnCreatePrintout() override;
	virtual void OnCreateToolbar(wxAuiToolBar* toolbar) override;

	void OnMenuEvent(wxCommandEvent& event);

	CGridEditor* m_gridEditor;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS(CSpreadsheetEditView);
};

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

#include "backend/metaCollection/metaSpreadsheetObject.h"

class FRONTEND_API ISpreadsheetDocument : public CMetaDocument {
public:

	virtual wxIcon GetIcon() const {
		return CBackendPicture::GetPictureAsIcon(g_metaCommonTemplateCLSID);
	}

	ISpreadsheetDocument() : CMetaDocument() {}
	virtual wxCommandProcessor* OnCreateCommandProcessor() override;
	
	virtual CGridEditor* GetGridCtrl() const;

private:
	wxDECLARE_NO_COPY_CLASS(ISpreadsheetDocument);
	wxDECLARE_ABSTRACT_CLASS(ISpreadsheetDocument);
};

// ----------------------------------------------------------------------------
// A very simple text document class
// ----------------------------------------------------------------------------

class FRONTEND_API CSpreadsheetFileDocument : public ISpreadsheetDocument {
public:

	CSpreadsheetFileDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument = wxObjectDataPtr<CBackendSpreadsheetObject>(new CBackendSpreadsheetObject)) : ISpreadsheetDocument(), m_spreadSheetDocument(spreadSheetDocument) { m_childDoc = false; }

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

	wxObjectDataPtr<CBackendSpreadsheetObject> m_spreadSheetDocument;

private:

	wxDECLARE_NO_COPY_CLASS(CSpreadsheetFileDocument);
	wxDECLARE_DYNAMIC_CLASS(CSpreadsheetFileDocument);
};

class FRONTEND_API CSpreadsheetEditDocument : public ISpreadsheetDocument {
public:

	CSpreadsheetEditDocument() : ISpreadsheetDocument() {}

	virtual bool OnCreate(const wxString& path, long flags) override;
	virtual bool SaveAs() override;

protected:

	virtual bool DoSaveDocument(const wxString& filename) override;

private:

	wxDECLARE_NO_COPY_CLASS(CSpreadsheetEditDocument);
	wxDECLARE_DYNAMIC_CLASS(CSpreadsheetEditDocument);
};

#endif