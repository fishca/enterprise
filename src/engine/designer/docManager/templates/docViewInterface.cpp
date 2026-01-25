#include "docViewInterface.h"

// ----------------------------------------------------------------------------
// CInterfaceEditView implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CInterfaceEditView, CMetaView);

wxBEGIN_EVENT_TABLE(CInterfaceEditView, CMetaView)
wxEND_EVENT_TABLE()

#include "win/editor/interfaceEditor/interfaceEditor.h"

bool CInterfaceEditView::OnCreate(CMetaDocument* doc, long flags)
{
	m_interfaceEditor = new CInterfaceEditor(m_viewFrame, wxID_ANY, doc->GetMetaObject());
	m_interfaceEditor->SetReadOnly(flags == wxDOC_READONLY);

	m_interfaceEditor->RefreshInterface();
	return CMetaView::OnCreate(doc, flags);
}

void CInterfaceEditView::OnUpdate(wxView* sender, wxObject* hint)
{
	if (m_interfaceEditor != nullptr) 
		m_interfaceEditor->RefreshInterface();	
}

void CInterfaceEditView::OnDraw(wxDC* WXUNUSED(dc))
{
	// nothing to do here, wxGrid draws itself
}

bool CInterfaceEditView::OnClose(bool deleteWindow)
{
	Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	if (CMetaView::OnClose(deleteWindow)) {
		m_interfaceEditor->Freeze();
		m_interfaceEditor->Destroy();
	}

	return true;
}

// ----------------------------------------------------------------------------
// CInterfaceDocument: wxDocument and wxGrid married
// ----------------------------------------------------------------------------

wxIMPLEMENT_CLASS(CInterfaceDocument, CMetaDocument);

bool CInterfaceDocument::OnCreate(const wxString& path, long flags)
{
	if (!CMetaDocument::OnCreate(path, flags))
		return false;

	return true;
}

// Since text windows have their own method for saving to/loading from files,
// we override DoSave/OpenDocument instead of Save/LoadObject
bool CInterfaceDocument::DoSaveDocument(const wxString& filename)
{
	return true;
}

bool CInterfaceDocument::DoOpenDocument(const wxString& filename)
{
	return true;
}

bool CInterfaceDocument::IsModified() const
{
	return CMetaDocument::IsModified();
}

void CInterfaceDocument::Modify(bool modified)
{
	CMetaDocument::Modify(modified);
}

// ----------------------------------------------------------------------------
// CInterfaceEditDocument implementation
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(CInterfaceEditDocument, CMetaDocument);