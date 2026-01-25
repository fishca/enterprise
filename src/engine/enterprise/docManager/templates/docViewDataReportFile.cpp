#include "docViewDataReportFile.h"

// ----------------------------------------------------------------------------
// CTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CReportEditView, CMetaView);

bool CReportEditView::OnCreate(CMetaDocument *doc, long flags)
{
	return CMetaView::OnCreate(doc, flags);
}

void CReportEditView::OnDraw(wxDC *WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool CReportEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow && GetFrame()) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	return CMetaView::OnClose(deleteWindow);
}

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CReportFileDocument, CMetaDocument);

bool CReportFileDocument::OnCreate(const wxString& path, long flags)
{
	/*if (!CMetaDocument::OnCreate(path, flags))
		return false;*/

	m_metaData = new CMetaDataReport();
	return true;
}

#include "frontend/mainFrame/objinspect/objinspect.h"

bool CReportFileDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	return wxDocument::OnCloseDocument();
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CReportFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadFromFile(filename))
		return false;

	//We must delete document after initialization
	return false;
}

bool CReportFileDocument::DoSaveDocument(const wxString& filename)
{
	/*if (!m_metaData->SaveToFile(filename))
		return false;*/

	return true;
}

bool CReportFileDocument::IsModified() const
{
	return CMetaDocument::IsModified();
}

void CReportFileDocument::Modify(bool modified)
{
	CMetaDocument::Modify(modified);
}
