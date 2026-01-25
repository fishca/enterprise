#include "docViewSpreadsheet.h"
#include "frontend/mainFrame/mainFrame.h"

enum
{
	wxID_MERGE_CELLS = wxID_HIGHEST + 150,
	wxID_SECTION_ADD,
	wxID_SECTION_REMOVE,
	wxID_SHOW_CELLS,
	wxID_SHOW_HEADERS,
	wxID_SHOW_SECTIONS,
	wxID_BORDERS,
	wxID_DOCK_TABLE,
};

// ----------------------------------------------------------------------------
// CSpreadsheetEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CSpreadsheetEditView, CMetaView);

wxBEGIN_EVENT_TABLE(CSpreadsheetEditView, CMetaView)

EVT_MENU(wxID_COPY, CSpreadsheetEditView::OnCopy)
EVT_MENU(wxID_PASTE, CSpreadsheetEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, CSpreadsheetEditView::OnSelectAll)

EVT_MENU(wxID_MERGE_CELLS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SECTION_ADD, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SECTION_REMOVE, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_CELLS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_HEADERS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_SECTIONS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_BORDERS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_DOCK_TABLE, CSpreadsheetEditView::OnMenuEvent)

wxEND_EVENT_TABLE()

bool CSpreadsheetEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_gridEditor = new CGridEditor(doc, m_viewFrame, wxID_ANY);
	m_gridEditor->EnableEditing(flags != wxDOC_READONLY);

	return CMetaView::OnCreate(doc, flags);
}

void CSpreadsheetEditView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) m_gridEditor->ActivateEditor();
}

void CSpreadsheetEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxGrid draws itself
}

#include "frontend/win/editor/gridEditor/gridPrintout.h"

wxPrintout* CSpreadsheetEditView::OnCreatePrintout()
{
	return new CGridEditorPrintout(m_gridEditor, wxGP_SHOW_NONE);
}

#include "frontend/artProvider/artProvider.h"

void CSpreadsheetEditView::OnCreateToolbar(wxAuiToolBar* toolbar)
{
	toolbar->AddTool(wxID_MERGE_CELLS, _("Merge cells"), wxArtProvider::GetBitmap(wxART_MERGE_CELL, wxART_DOC_TEMPLATE), _("Merge cells"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_MERGE_CELLS, m_gridEditor->IsEditable());
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_SECTION_ADD, _("Add section"), wxArtProvider::GetBitmap(wxART_ADD_SECTION, wxART_DOC_TEMPLATE), _("Add"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_SECTION_ADD, m_gridEditor->IsEditable());
	toolbar->AddTool(wxID_SECTION_REMOVE, _("Remove section"), wxArtProvider::GetBitmap(wxART_REMOVE_SECTION, wxART_DOC_TEMPLATE), _("Remove"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_SECTION_REMOVE, m_gridEditor->IsEditable());
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_SHOW_CELLS, _("Show cells"), wxArtProvider::GetBitmap(wxART_SHOW_CELL, wxART_DOC_TEMPLATE), _("Show cells"));
	toolbar->AddTool(wxID_SHOW_HEADERS, _("Show headers"), wxArtProvider::GetBitmap(wxART_SHOW_HEADER, wxART_DOC_TEMPLATE), _("Show headers"));
	toolbar->AddTool(wxID_SHOW_SECTIONS, _("Show sections"), wxArtProvider::GetBitmap(wxART_SHOW_SECTION, wxART_DOC_TEMPLATE), _("Show sections"));
	toolbar->AddTool(wxID_BORDERS, _("Borders"), wxArtProvider::GetBitmap(wxART_BORDER, wxART_DOC_TEMPLATE), _("Borders"));
	toolbar->SetToolDropDown(wxID_BORDERS, true);
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_DOCK_TABLE, _("Dock table"), wxArtProvider::GetBitmap(wxART_DOCK_TABLE, wxART_DOC_TEMPLATE), _("Dock table"));
}

bool CSpreadsheetEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_gridEditor->Freeze();
		return m_gridEditor->Destroy();
	}

	return false;
}

void CSpreadsheetEditView::OnMenuEvent(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case wxID_MERGE_CELLS:
		m_gridEditor->MergeCells();
		break;
	case wxID_SECTION_ADD:
		m_gridEditor->AddArea();
		break;
	case wxID_SECTION_REMOVE:
		m_gridEditor->DeleteArea();
		break;
	case wxID_SHOW_CELLS:
		m_gridEditor->ShowCells();
		break;
	case wxID_SHOW_HEADERS:
		m_gridEditor->ShowHeader();
		break;
	case wxID_SHOW_SECTIONS:
		m_gridEditor->ShowArea();
		break;
	case wxID_DOCK_TABLE:
		m_gridEditor->DockTable();
		break;
	}

	//event.Skip();
}

// ----------------------------------------------------------------------------
// ISpreadsheetDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

wxIMPLEMENT_ABSTRACT_CLASS(ISpreadsheetDocument, CMetaDocument);

wxIMPLEMENT_DYNAMIC_CLASS(CSpreadsheetFileDocument, ISpreadsheetDocument);
wxIMPLEMENT_DYNAMIC_CLASS(CSpreadsheetEditDocument, ISpreadsheetDocument);

CGridEditor* ISpreadsheetDocument::GetGridCtrl() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CSpreadsheetEditView)->GetGridCtrl() : nullptr;
}

// ----------------------------------------------------------------------------
// CSpreadsheetFileDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

bool CSpreadsheetFileDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return GetGridCtrl()->LoadDocument(m_spreadSheetDocument);
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CSpreadsheetFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_spreadSheetDocument.LoadFromFile(filename))
		return false;

	return GetGridCtrl()->LoadDocument(m_spreadSheetDocument);
}

bool CSpreadsheetFileDocument::DoSaveDocument(const wxString& filename)
{
	if (!GetGridCtrl()->SaveDocument(m_spreadSheetDocument))
		return false;

	return m_spreadSheetDocument.SaveToFile(filename);
}

// ----------------------------------------------------------------------------
// CSpreadsheetEditDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

bool CSpreadsheetEditDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	const IMetaObjectSpreadsheet* creator = m_metaObject->ConvertToType<IMetaObjectSpreadsheet>();
	if (creator != nullptr)
		return GetGridCtrl()->LoadDocument(creator->GetSpreadsheetDesc());

	return false;
}
