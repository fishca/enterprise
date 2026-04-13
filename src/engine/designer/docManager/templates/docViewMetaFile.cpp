#include "docViewMetaFile.h"

// ----------------------------------------------------------------------------
// ibTextEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibMetadataEditView, ibMetaView);

bool ibMetadataEditView::OnCreate(ibMetaDocument* doc, long flags)
{
	m_metaTree = new ibMetadataTree(doc, m_viewFrame);
	m_metaTree->SetReadOnly(true);

	return ibMetaView::OnCreate(doc, flags);
}

void ibMetadataEditView::OnActivateView(bool activate, wxView* activeView, wxView* deactiveView)
{
	if (activate) m_metaTree->ActivateTree();
}

void ibMetadataEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, ibMetadataTree draws itself
}

#include "docManager/docManager.h"

bool ibMetadataEditView::OnClose(bool deleteWindow)
{
	//Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (ibMetaView::OnClose(deleteWindow)) {
		
		m_metaTree->Freeze();
		m_metaTree->Destroy();

		m_metaTree = nullptr;

		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

#include "frontend/mainFrame/mainFrame.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibMetadataBrowserDocument, ibMetaDocument);

bool ibMetadataBrowserDocument::OnCreate(const wxString& path, long flags)
{
	if (!ibMetaDocument::OnCreate(path, flags))
		return false;
	
	return GetMetaTree()->Load(m_metaData);
}

bool ibMetadataBrowserDocument::OnCloseDocument()
{
	objectInspector->SelectObject(activeMetaData->GetCommonMetaObject());
	return true;
}

// ----------------------------------------------------------------------------

ibMetadataTree* ibMetadataBrowserDocument::GetMetaTree() const
{
	wxView* view = GetFirstView();
	return view ? wxDynamicCast(view, ibMetadataEditView)->GetMetaTree() : nullptr;
}

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(ibMetadataFilibDocument, ibMetaDocument);

bool ibMetadataFilibDocument::OnCreate(const wxString& path, long flags)
{
	m_metaData = new ibMetaDataConfigurationFile();

	if (!ibMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool ibMetadataFilibDocument::DoOpenDocument(const wxString& filename)
{
	if (!m_metaData->LoadConfigFromFile(filename))
		return false;

	if (!GetMetaTree()->Load(m_metaData))
		return false;

	return m_metaData->RunDatabase(onlyLoadFlag);
}

bool ibMetadataFilibDocument::OnCloseDocument()
{
	if (!m_metaData->CloseDatabase(forceCloseFlag)) {
		return false;
	}

	objectInspector->SelectObject(activeMetaData->GetCommonMetaObject());
	return true;
}

bool ibMetadataFilibDocument::DoSaveDocument(const wxString& filename)
{
	/*if (!m_metaData->SaveToFile(filename))
		return false;*/

	return true;
}

bool ibMetadataFilibDocument::IsModified() const
{
	return false;
}

void ibMetadataFilibDocument::Modify(bool modified)
{
}