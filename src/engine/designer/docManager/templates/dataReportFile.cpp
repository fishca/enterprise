#include "dataReportFile.h"

wxIMPLEMENT_DYNAMIC_CLASS(CReportView, CMetaView);

bool CReportView::OnCreate(CMetaDocument *doc, long flags)
{
	m_metaTree = new CDataReportTree(doc, m_viewFrame);
	m_metaTree->SetReadOnly(false);

	return CMetaView::OnCreate(doc, flags);
}

void CReportView::OnDraw(wxDC *WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool CReportView::OnClose(bool deleteWindow)
{
	Activate(false);

	if (deleteWindow)
	{
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_metaTree->Freeze();
		return m_metaTree->Destroy();
	}

	return false;
}

wxIMPLEMENT_DYNAMIC_CLASS(CReportFileDocument, CMetaDocument);

bool CReportFileDocument::OnCreate(const wxString& path, long flags)
{
	m_metaData = new CMetaDataReport();
	if (!CMetaDocument::OnCreate(path, flags))
		return false;
	return true;
}

#include "frontend/mainFrame/mainFrame.h"

bool CReportFileDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	objectInspector->SelectObject(commonMetaData->GetCommonMetaObject());
	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CReportFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadFromFile(filename))
		return false;

	if (!GetMetaTree()->Load(m_metaData))
		return false;

	return true;
}

bool CReportFileDocument::DoSaveDocument(const wxString& filename)
{
	if (!GetMetaTree()->Save())
		return false;

	if (!m_metaData->SaveToFile(filename))
		return false;

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

CDataReportTree *CReportFileDocument::GetMetaTree() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CReportView)->GetMetaTree() : nullptr;
}
