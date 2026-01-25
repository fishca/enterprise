#include "docViewDataProcessorFile.h"

// ----------------------------------------------------------------------------
// CTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CDataProcessorEditView, CMetaView);

bool CDataProcessorEditView::OnCreate(CMetaDocument *doc, long flags)
{
	return CMetaView::OnCreate(doc, flags);
}

void CDataProcessorEditView::OnDraw(wxDC *WXUNUSED(dc))
{
	// nothing to do here, wxTextCtrl draws itself
}

bool CDataProcessorEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	return CMetaView::OnClose(deleteWindow);
}

// ----------------------------------------------------------------------------
// ITextDocument: CDataProcessorFileDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CDataProcessorFileDocument, CMetaDocument);
\
bool CDataProcessorFileDocument::OnCreate(const wxString& path, long flags)
{
	/*if (!CMetaDocument::OnCreate(path, flags))
		return false;*/
	m_metaData = new CMetaDataDataProcessor();
	return true;
}

#include "frontend/mainFrame/objinspect/objinspect.h"

bool CDataProcessorFileDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	return wxDocument::OnCloseDocument();
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CDataProcessorFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadFromFile(filename))
		return false;
	
	//We must delete document after initialization
	return false;
}

bool CDataProcessorFileDocument::DoSaveDocument(const wxString& filename)
{
	/*if (!m_metaData->SaveToFile(filename))
		return false;*/

	return true;
}

bool CDataProcessorFileDocument::IsModified() const
{
	return CMetaDocument::IsModified();
}

void CDataProcessorFileDocument::Modify(bool modified)
{
	CMetaDocument::Modify(modified);
}