#include "docViewSpreadsheet.h"
#include "frontend/mainFrame/mainFrame.h"

enum
{
	wxID_MERGE_CELL = wxID_HIGHEST + 150,
	wxID_AREA_ADD,
	wxID_AREA_DELETE,
	wxID_FREEZE_ROW,
	wxID_FREEZE_COL,
	wxID_PRINT_BRAKE_ROW_ADD,
	wxID_PRINT_BRAKE_ROW_DELETE,
	wxID_PRINT_BRAKE_COL_ADD,
	wxID_PRINT_BRAKE_COL_DELETE,
	wxID_SHOW_CELL,
	wxID_SHOW_HEADER,
	wxID_SHOW_AREA,
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
EVT_MENU(wxID_MERGE_CELL, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_AREA_ADD, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_AREA_DELETE, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_FREEZE_ROW, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_FREEZE_COL, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_PRINT_BRAKE_ROW_ADD, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_PRINT_BRAKE_ROW_DELETE, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_PRINT_BRAKE_COL_ADD, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_PRINT_BRAKE_COL_DELETE, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_CELL, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_HEADER, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_SHOW_AREA, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_BORDERS, CSpreadsheetEditView::OnMenuEvent)
EVT_MENU(wxID_DOCK_TABLE, CSpreadsheetEditView::OnMenuEvent)

wxEND_EVENT_TABLE()

bool CSpreadsheetEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_gridEditor = new CGridEditor(doc, m_viewFrame, wxID_ANY);
	m_gridEditor->EnableEditing(flags != wxDOC_READONLY);
	m_gridEditor->EnableGridArea(doc->ConvertMetaObjectToType<IValueMetaObjectSpreadsheet>());

	return CMetaView::OnCreate(doc, flags);
}

#if wxUSE_MENUS	
wxMenuBar* CSpreadsheetEditView::CreateMenuBar() const
{
	wxMenuBar* mb = new wxMenuBar;

	wxMenu* menu = new wxMenu;
	wxMenuItem* menuItem = nullptr;

	wxMenu* menuArea = new wxMenu;
	menuItem = menuArea->Append(wxID_AREA_ADD, _("Add area"));
	menuItem = menuArea->Append(wxID_AREA_DELETE, _("Delete area"));
	menu->AppendSubMenu(menuArea, _("Area"));

	wxMenu* menuFreeze = new wxMenu;
	menuItem = menuFreeze->Append(wxID_FREEZE_ROW, _("Freeze row"));
	menuItem = menuFreeze->Append(wxID_FREEZE_COL, _("Freeze column"));
	menu->AppendSubMenu(menuFreeze, _("Freeze"));

	wxMenu* menuPrintBrake = new wxMenu;
	menuItem = menuPrintBrake->Append(wxID_PRINT_BRAKE_ROW_ADD, _("Add brake row"));
	menuItem = menuPrintBrake->Append(wxID_PRINT_BRAKE_COL_ADD, _("Add brake column"));
	menuItem = menuPrintBrake->Append(wxID_PRINT_BRAKE_ROW_DELETE, _("Delete brake row"));
	menuItem = menuPrintBrake->Append(wxID_PRINT_BRAKE_COL_DELETE, _("Delete brake column"));
	menu->AppendSubMenu(menuPrintBrake, _("Print brake"));

	menuItem = menu->AppendSeparator();
	menuItem = menu->Append(wxID_SHOW_CELL, _("Show cells"));
	menuItem = menu->Append(wxID_SHOW_HEADER, _("Show headers"));
	menuItem = menu->Append(wxID_SHOW_AREA, _("Show area"));

	menuItem = menu->AppendSeparator();
	menuItem = menu->Append(wxID_MERGE_CELL, _("Merge cells"));

	mb->Append(menu, _("Spreadsheet"));
	return mb;
}
#endif 

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
	return m_gridEditor->CreatePrintout();
}

#include "frontend/artProvider/artProvider.h"

void CSpreadsheetEditView::OnCreateToolbar(wxAuiToolBar* toolbar)
{
	toolbar->AddTool(wxID_MERGE_CELL, _("Merge cells"), wxArtProvider::GetBitmap(wxART_MERGE_CELL, wxART_DOC_TEMPLATE), _("Merge cells"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_MERGE_CELL, m_gridEditor->IsEditable());
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_AREA_ADD, _("Add section"), wxArtProvider::GetBitmap(wxART_ADD_SECTION, wxART_DOC_TEMPLATE), _("Add"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_AREA_ADD, m_gridEditor->IsEditable());
	toolbar->AddTool(wxID_AREA_DELETE, _("Delete area"), wxArtProvider::GetBitmap(wxART_REMOVE_SECTION, wxART_DOC_TEMPLATE), _("Remove"), wxItemKind::wxITEM_NORMAL);
	toolbar->EnableTool(wxID_AREA_DELETE, m_gridEditor->IsEditable());
	toolbar->AddSeparator();
	toolbar->AddTool(wxID_SHOW_CELL, _("Show cells"), wxArtProvider::GetBitmap(wxART_SHOW_CELL, wxART_DOC_TEMPLATE), _("Show cells"));
	toolbar->AddTool(wxID_SHOW_HEADER, _("Show headers"), wxArtProvider::GetBitmap(wxART_SHOW_HEADER, wxART_DOC_TEMPLATE), _("Show headers"));
	toolbar->AddTool(wxID_SHOW_AREA, _("Show area"), wxArtProvider::GetBitmap(wxART_SHOW_SECTION, wxART_DOC_TEMPLATE), _("Show area"));
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
	wxArrayInt arrRows = m_gridEditor->GetSelectedRows();
	const int selected_row = arrRows.Count() > 0 ? arrRows[0] : -1;

	wxArrayInt arrCols = m_gridEditor->GetSelectedCols();
	const int selected_col = arrCols.Count() > 0 ? arrCols[0] : -1;

	switch (event.GetId())
	{
	case wxID_MERGE_CELL:
		m_gridEditor->MergeCells();
		break;
	case wxID_AREA_ADD:
		m_gridEditor->AddArea();
		break;
	case wxID_AREA_DELETE:
		m_gridEditor->DeleteArea();
		break;
	case wxID_FREEZE_ROW:
		if (selected_row >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->FreezeTo(selected_row, 0);
		break;
	case wxID_FREEZE_COL:
		if (selected_col >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->FreezeTo(0, selected_col);
		break;
	case wxID_PRINT_BRAKE_ROW_ADD:
		if (selected_row >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->AddRowBrake(selected_row);
		break;
	case wxID_PRINT_BRAKE_COL_ADD:
		if (selected_col >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->AddColBrake(selected_col);
		break;
	case wxID_PRINT_BRAKE_ROW_DELETE:
		if (selected_row >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->DeleteRowBrake(selected_row);
		break;
	case wxID_PRINT_BRAKE_COL_DELETE:
		if (selected_col >= 0 && m_gridEditor->IsSelection())
			m_gridEditor->DeleteColBrake(selected_col);
		break;
	case wxID_SHOW_CELL:
		m_gridEditor->ShowCells();
		break;
	case wxID_SHOW_HEADER:
		m_gridEditor->ShowHeader();
		break;
	case wxID_SHOW_AREA:
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

wxCommandProcessor* ISpreadsheetDocument::OnCreateCommandProcessor()
{
	return new wxGridExtCommandProcessor(GetGridCtrl());
}

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

	return GetGridCtrl()->AssociateDocument(m_spreadSheetDocument);
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CSpreadsheetFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_spreadSheetDocument->LoadFromFile(filename))
		return false;

	return GetGridCtrl()->LoadDocument(m_spreadSheetDocument);
}

bool CSpreadsheetFileDocument::DoSaveDocument(const wxString& filename)
{
	if (!GetGridCtrl()->SaveDocument(m_spreadSheetDocument))
		return false;

	return m_spreadSheetDocument->SaveToFile(filename);
}

// ----------------------------------------------------------------------------
// CSpreadsheetEditDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

bool CSpreadsheetEditDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	const IValueMetaObjectSpreadsheet* creator = m_metaObject->ConvertToType<IValueMetaObjectSpreadsheet>();
	if (creator != nullptr)
		return GetGridCtrl()->LoadDocument(creator->GetSpreadsheetDesc());

	return false;
}

bool CSpreadsheetEditDocument::SaveAs()
{
	wxDocTemplate* docTemplate = GetDocumentTemplate();
	if (!docTemplate)
		return false;

	const IValueMetaObjectSpreadsheet* creator = m_metaObject->ConvertToType<IValueMetaObjectSpreadsheet>();
	if (creator == nullptr)
		return false;

#ifdef wxHAS_MULTIPLE_FILEDLG_FILTERS
	wxString filter = docTemplate->GetDescription() + wxT(" (") +
		docTemplate->GetFileFilter() + wxT(")|") +
		docTemplate->GetFileFilter();

	// Now see if there are some other template with identical view and document
	// classes, whose filters may also be used.
	if (docTemplate->GetViewClassInfo() && docTemplate->GetDocClassInfo())
	{
		wxList::compatibility_iterator
			node = docTemplate->GetDocumentManager()->GetTemplates().GetFirst();
		while (node)
		{
			wxDocTemplate* t = (wxDocTemplate*)node->GetData();

			if (t->IsVisible() && t != docTemplate &&
				t->GetViewClassInfo() == docTemplate->GetViewClassInfo() &&
				t->GetDocClassInfo() == docTemplate->GetDocClassInfo())
			{
				// add a '|' to separate this filter from the previous one
				if (!filter.empty())
					filter << wxT('|');

				filter << t->GetDescription()
					<< wxT(" (") << t->GetFileFilter() << wxT(") |")
					<< t->GetFileFilter();
			}

			node = node->GetNext();
		}
	}
#else
	wxString filter = docTemplate->GetFileFilter();
#endif

	wxString defaultDir = docTemplate->GetDirectory();
	if (defaultDir.empty())
	{
		defaultDir = wxPathOnly(creator->GetName());
		if (defaultDir.empty())
			defaultDir = GetDocumentManager()->GetLastDirectory();
	}

	wxString fileName = wxFileSelector(_("Save As"),
		defaultDir,
		wxFileNameFromPath(creator->GetName()),
		docTemplate->GetDefaultExtension(),
		filter,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		GetDocumentWindow());

	if (fileName.empty())
		return false; // cancelled by user

	// Files that were not saved correctly are not added to the FileHistory.
	if (!DoSaveDocument(fileName))
		return false;

	// A file that doesn't use the default extension of its document template
	// cannot be opened via the FileHistory, so we do not add it.
	if (docTemplate->FileMatchesTemplate(fileName))
	{
		GetDocumentManager()->AddFileToHistory(fileName);
	}
	//else: the user will probably not be able to open the file again, so we
	//      could warn about the wrong file-extension here

	return true;
}

bool CSpreadsheetEditDocument::DoSaveDocument(const wxString& filename)
{
	wxObjectDataPtr<CBackendSpreadsheetObject>spreadSheetDocument;

	if (!GetGridCtrl()->GetActiveDocument(spreadSheetDocument))
		return false;

	return spreadSheetDocument->SaveToFile(filename);
}
