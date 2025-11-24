#include "template.h"
#include "frontend/mainFrame/mainFrame.h"

enum
{
	wxID_MERGE_CELLS = wxID_HIGHEST + 100,
	wxID_SECTION_ADD,
	wxID_SECTION_REMOVE,
	wxID_SHOW_CELLS,
	wxID_SHOW_HEADERS,
	wxID_SHOW_SECTIONS,
	wxID_BORDERS,
	wxID_DOCK_TABLE,
};

// ----------------------------------------------------------------------------
// CGridEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CGridEditView, CMetaView);

wxBEGIN_EVENT_TABLE(CGridEditView, CMetaView)
EVT_MENU(wxID_COPY, CGridEditView::OnCopy)
EVT_MENU(wxID_PASTE, CGridEditView::OnPaste)
EVT_MENU(wxID_SELECTALL, CGridEditView::OnSelectAll)

EVT_MENU(wxID_MERGE_CELLS, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_SECTION_ADD, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_SECTION_REMOVE, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_CELLS, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_HEADERS, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_BORDERS, CGridEditView::OnMenuEvent)
EVT_MENU(wxID_DOCK_TABLE, CGridEditView::OnMenuEvent)

wxEND_EVENT_TABLE()

bool CGridEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_gridEditor = new CGrid(m_viewFrame, wxID_ANY);
	m_gridEditor->EnableEditing(flags != wxDOC_READONLY);

	return CMetaView::OnCreate(doc, flags);
}

void CGridEditView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) m_gridEditor->ActivateEditor();
}

void CGridEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxGrid draws itself
}

#include "frontend/mainFrame/grid/printout/gridPrintout.h"

wxPrintout* CGridEditView::OnCreatePrintout()
{
	return new CGridPrintout(m_gridEditor, wxGP_SHOW_NONE);
}

#include "frontend/artProvider/artProvider.h"

void CGridEditView::OnCreateToolbar(wxAuiToolBar* toolbar)
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

bool CGridEditView::OnClose(bool deleteWindow)
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

void CGridEditView::OnMenuEvent(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case wxID_MERGE_CELLS:
		m_gridEditor->MergeCells();
		break;
	case wxID_SECTION_ADD:
		m_gridEditor->AddSection();
		break;
	case wxID_SECTION_REMOVE:
		m_gridEditor->RemoveSection();
		break;
	case wxID_SHOW_CELLS:
		m_gridEditor->ShowCells();
		break;
	case wxID_SHOW_HEADERS:
		m_gridEditor->ShowHeader();
		break;
	case wxID_SHOW_SECTIONS:
		m_gridEditor->ShowSection();
		break;
	case wxID_DOCK_TABLE:
		m_gridEditor->DockTable();
		break;
	}

	//event.Skip();
}

// ----------------------------------------------------------------------------
// CGridDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(CGridDocument, CMetaDocument);

bool CGridDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CGridDocument::DoSaveDocument(const wxString& filename)
{
	return true;
}

bool CGridDocument::DoOpenDocument(const wxString& filename)
{
	return true;
}

bool CGridDocument::IsModified() const
{
	return CMetaDocument::IsModified();
}

void CGridDocument::Modify(bool modified)
{
	CMetaDocument::Modify(modified);
}

// ----------------------------------------------------------------------------
// CGridEditDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CGridEditDocument, CMetaDocument);

wxGrid* CGridEditDocument::GetGridCtrl() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CGridEditView)->GetGridCtrl() : nullptr;
}