////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor 
////////////////////////////////////////////////////////////////////////////

#include "visualEditor.h"

#include "backend/metaCollection/metaFormObject.h"

void CVisualEditorNotebook::CVisualEditor::Execute(CVisualEditorCmd* cmd)
{
	if (m_cmdProc != nullptr) m_cmdProc->Execute(cmd);
	NotifyEditorSaved();
}

//////////////////////////////////////////////////////////////////////////////////////

void CVisualEditorNotebook::CVisualEditor::NotifyEditorLoaded()
{
	m_objectTree->OnEditorLoaded();
}

void CVisualEditorNotebook::CVisualEditor::NotifyEditorSaved()
{
	IMetaObjectForm* creator = m_document->ConvertMetaObjectToType<IMetaObjectForm>();
	wxASSERT(creator);
	// Create a std::string and copy your document data in to the string    
	if (creator != nullptr) creator->SaveFormData(m_valueForm);	
}

void CVisualEditorNotebook::CVisualEditor::NotifyEditorRefresh()
{
	m_objectTree->OnEditorRefresh();
}

void CVisualEditorNotebook::CVisualEditor::NotifyObjectCreated(IValueFrame* obj)
{
	m_objectTree->OnObjectCreated(obj);
}

void CVisualEditorNotebook::CVisualEditor::NotifyObjectSelected(IValueFrame* obj, bool force)
{
	m_objectTree->OnObjectSelected(obj);
}

void CVisualEditorNotebook::CVisualEditor::NotifyObjectExpanded(IValueFrame* obj)
{
	m_objectTree->OnObjectExpanded(obj);
}

void CVisualEditorNotebook::CVisualEditor::NotifyObjectRemoved(IValueFrame* obj)
{
	m_objectTree->OnObjectRemoved(obj);
}

void CVisualEditorNotebook::CVisualEditor::NotifyPropertyModified(IProperty* prop)
{
	m_objectTree->OnPropertyModified(prop);
}

void CVisualEditorNotebook::CVisualEditor::NotifyEventModified(IEvent* event)
{
}

//////////////////////////////////////////////////////////////////////////////////////

wxObject* CVisualEditorNotebook::CVisualEditor::CVisualEditorHost::Create(IValueFrame* control, wxWindow* wxparent)
{
	return control->Create(wxparent, this);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorHost::OnCreated(IValueFrame* control, wxObject* obj, wxWindow* wndParent, bool firstÑreated)
{
	control->OnCreated(obj, wndParent, this, firstÑreated);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorHost::Update(IValueFrame* control, wxObject* obj)
{
	control->Update(obj, this);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorHost::OnUpdated(IValueFrame* control, wxObject* obj, wxWindow* wndParent)
{
	control->OnUpdated(obj, wndParent, this);
}

void CVisualEditorNotebook::CVisualEditor::CVisualEditorHost::Cleanup(IValueFrame* control, wxObject* obj)
{
	control->Cleanup(obj, this);
}