#include "metaFile.h"

// ----------------------------------------------------------------------------
// CTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CMetadataView, CMetaView);

bool CMetadataView::OnCreate(CMetaDocument* doc, long flags)
{
	m_metaTree = new CMetadataTree(doc, m_viewFrame);
	m_metaTree->SetReadOnly(true);

	return CMetaView::OnCreate(doc, flags);
}

void CMetadataView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) m_metaTree->Activate();
}

void CMetadataView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, CMetadataTree draws itself
}

#include "docManager/docManager.h"

bool CMetadataView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_metaTree->Freeze();
		return m_metaTree->Destroy();
	}

	return false;
}

// ----------------------------------------------------------------------------
// CTextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

#include "frontend/mainFrame/mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(CMetadataBrowserDocument, CMetaDocument);

bool CMetadataBrowserDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;
	
	return GetMetaTree()->Load(m_metaData);
}

bool CMetadataBrowserDocument::OnCloseDocument()
{
	objectInspector->SelectObject(commonMetaData->GetCommonMetaObject());
	return true;
}

// ----------------------------------------------------------------------------

CMetadataTree* CMetadataBrowserDocument::GetMetaTree() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, CMetadataView)->GetMetaTree() : nullptr;
}

// ----------------------------------------------------------------------------
// CTextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CMetadataFileDocument, CMetaDocument);

bool CMetadataFileDocument::OnCreate(const wxString& path, long flags)
{
	m_metaData = new CMetaDataConfigurationFile();

	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CMetadataFileDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadFromFile(filename))
		return false;

	if (!GetMetaTree()->Load(m_metaData))
		return false;

	return m_metaData->RunDatabase(onlyLoadFlag);
}

bool CMetadataFileDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	objectInspector->SelectObject(commonMetaData->GetCommonMetaObject());
	return true;
}

bool CMetadataFileDocument::DoSaveDocument(const wxString& filename)
{
	/*if (!m_metaData->SaveToFile(filename))
		return false;*/

	return true;
}

bool CMetadataFileDocument::IsModified() const
{
	return false;
}

void CMetadataFileDocument::Modify(bool modified)
{
}