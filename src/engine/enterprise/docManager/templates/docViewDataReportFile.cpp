#include "docViewDataReportFile.h"

// ----------------------------------------------------------------------------
// ibTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibReportEditView, ibMetaView);

bool ibReportEditView::OnCreate(ibMetaDocument *doc, long flags)
{
	return ibMetaView::OnCreate(doc, flags);
}

void ibReportEditView::OnDraw(wxDC *WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool ibReportEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow && GetFrame()) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	return ibMetaView::OnClose(deleteWindow);
}

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibReportFilibDocument, ibMetaDocument);

bool ibReportFilibDocument::OnCreate(const wxString& path, long flags)
{
	/*if (!ibMetaDocument::OnCreate(path, flags))
		return false;*/

	m_metaData = new ibMetaDataReport();
	return true;
}

#include "frontend/mainFrame/objinspect/objinspect.h"

bool ibReportFilibDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	return wxDocument::OnCloseDocument();
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool ibReportFilibDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadFromFile(filename))
		return false;

	//We must delete document after initialization
	return false;
}

bool ibReportFilibDocument::DoSaveDocument(const wxString& filename)
{
	/*if (!m_metaData->SaveToFile(filename))
		return false;*/

	return true;
}

bool ibReportFilibDocument::IsModified() const
{
	return ibMetaDocument::IsModified();
}

void ibReportFilibDocument::Modify(bool modified)
{
	ibMetaDocument::Modify(modified);
}
