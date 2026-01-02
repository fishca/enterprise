#include "toolbar.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueToolBarItem, IValueControl);
wxIMPLEMENT_DYNAMIC_CLASS(CValueToolBarSeparator, IValueControl);

//***********************************************************************************
//*                           CValueToolBarItem                               *
//***********************************************************************************

CValueToolBarItem::CValueToolBarItem() : IValueControl()
{
}

void CValueToolBarItem::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(wxparent);
	wxASSERT(toolbar);
	wxAuiToolBarItem* toolItem = toolbar->AddTool(GetControlID(),
		m_propertyTitle->GetValueAsTranslateString(),
		m_propertyPicture->GetValueAsBitmap(),
		wxNullBitmap,
		wxItemKind::wxITEM_NORMAL,
		m_properyTooltip->GetValueAsTranslateString(),
		wxEmptyString,
		wxobject
	);

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
	toolItem->SetUserData((long long)wxobject);
#else 
	toolItem->SetUserData((long)wxobject);
#endif 

	toolbar->EnableTool(toolItem->GetId(), m_propertyEnabled->GetValueAsBoolean());

	if (m_propertyContextMenu->GetValueAsBoolean() == true && !toolItem->HasDropDown())
		toolbar->SetToolDropDown(toolItem->GetId(), true);
	else if (m_propertyContextMenu->GetValueAsBoolean() == false && toolItem->HasDropDown())
		toolbar->SetToolDropDown(toolItem->GetId(), false);

	toolbar->Realize();
	toolbar->Refresh();
	toolbar->Update();
}

void CValueToolBarItem::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(wxparent);
	wxASSERT(toolbar);

	wxAuiToolBarItem* toolItem = toolbar->FindTool(GetControlID());
	IValueFrame* parentControl = GetParent(); int idx = wxNOT_FOUND;

	for (unsigned int i = 0; i < parentControl->GetChildCount(); i++) {
		IValueFrame* child = parentControl->GetChild(i);
		if (m_controlId == child->GetControlID()) {
			idx = i;
			break;
		}
	}

	if (toolItem != nullptr)
		toolbar->DestroyTool(GetControlID());

	if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Auto) {
		const CActionCollection& collection = GetOwner()->GetActionArray();
		if (GetItemRepresentation(collection) == enRepresentation::eRepresentation_PictureAndText) {
			toolItem = toolbar->InsertTool(idx, GetControlID(),
				GetItemCaption(collection),
				GetItemPicture(collection),
				wxNullBitmap,
				wxItemKind::wxITEM_NORMAL,
				GetItemToolTip(collection),
				wxEmptyString,
				wxobject
			);
		}
		else if (GetItemRepresentation(collection) == enRepresentation::eRepresentation_Picture) {
			toolItem = toolbar->InsertTool(idx, GetControlID(),
				wxEmptyString,
				GetItemPicture(collection),
				wxNullBitmap,
				wxItemKind::wxITEM_NORMAL,
				GetItemToolTip(collection),
				wxEmptyString,
				wxobject
			);
		}
		else if (GetItemRepresentation(collection) == enRepresentation::eRepresentation_Text) {
			toolItem = toolbar->InsertTool(idx, GetControlID(),
				GetItemCaption(collection),
				wxNullBitmap,
				wxNullBitmap,
				wxItemKind::wxITEM_NORMAL,
				GetItemToolTip(collection),
				wxEmptyString,
				wxobject
			);
		}
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_PictureAndText) {
		const CActionCollection& collection = GetOwner()->GetActionArray();
		toolItem = toolbar->InsertTool(idx, GetControlID(),
			GetItemCaption(collection),
			GetItemPicture(collection),
			wxNullBitmap,
			wxItemKind::wxITEM_NORMAL,
			GetItemToolTip(collection),
			wxEmptyString,
			wxobject
		);
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Picture) {
		const CActionCollection& collection = GetOwner()->GetActionArray();
		toolItem = toolbar->InsertTool(idx, GetControlID(),
			wxEmptyString,
			GetItemPicture(collection),
			wxNullBitmap,
			wxItemKind::wxITEM_NORMAL,
			GetItemToolTip(collection),
			wxEmptyString,
			wxobject
		);
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Text) {
		const CActionCollection& collection = GetOwner()->GetActionArray();
		toolItem = toolbar->InsertTool(idx, GetControlID(),
			GetItemCaption(collection),
			wxNullBitmap,
			wxNullBitmap,
			wxItemKind::wxITEM_NORMAL,
			GetItemToolTip(collection),
			wxEmptyString,
			wxobject
		);
	}

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
	toolItem->SetUserData((long long)wxobject);
#else 
	toolItem->SetUserData((long)wxobject);
#endif 

	toolbar->EnableTool(toolItem->GetId(), m_propertyEnabled->GetValueAsBoolean());

	if (m_propertyContextMenu->GetValueAsBoolean() == true && !toolItem->HasDropDown())
		toolbar->SetToolDropDown(toolItem->GetId(), true);
	else if (m_propertyContextMenu->GetValueAsBoolean() == false && toolItem->HasDropDown())
		toolbar->SetToolDropDown(toolItem->GetId(), false);

	toolbar->Realize();
	toolbar->Refresh();
	toolbar->Update();
}

void CValueToolBarItem::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(visualHost->GetWxObject(GetParent()));

	toolbar->DestroyTool(GetControlID());

	toolbar->Realize();
	toolbar->Refresh();
	toolbar->Update();
}

bool CValueToolBarItem::CanDeleteControl() const
{
	unsigned int toolCount = 0;
	for (unsigned int idx = 0; idx < m_parent->GetChildCount(); idx++) {
		auto child = m_parent->GetChild(idx);
		if (wxT("tool") == child->GetClassName()) {
			toolCount++;
		}
	}

	return toolCount > 1;
}

//***********************************************************************************
//*                           CValueToolBarSeparator                                *
//***********************************************************************************

CValueToolBarSeparator::CValueToolBarSeparator() : IValueControl()
{
}

void CValueToolBarSeparator::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(visualHost->GetWxObject(GetParent()));
	wxASSERT(toolbar);
	wxAuiToolBarItem* toolItem = toolbar->AddSeparator();
	toolItem->SetId(GetControlID());

	toolbar->Realize();
	toolbar->Refresh();
	toolbar->Update();
}

void CValueToolBarSeparator::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(visualHost->GetWxObject(GetParent()));
	wxASSERT(toolbar);

	wxAuiToolBarItem* toolItem = toolbar->FindTool(GetControlID());
	IValueFrame* m_parentControl = GetParent(); int idx = wxNOT_FOUND;

	for (unsigned int i = 0; i < m_parentControl->GetChildCount(); i++)
	{
		IValueFrame* child = m_parentControl->GetChild(i);
		if (m_controlId == child->GetControlID()) { idx = i; break; }
	}

	if (toolItem) { toolbar->DestroyTool(GetControlID()); }
	toolbar->InsertSeparator(idx, GetControlID());

	toolbar->Realize();
	toolbar->Refresh();
	toolbar->Update();
}

void CValueToolBarSeparator::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	CAuiToolBar* toolbar = dynamic_cast<CAuiToolBar*>(visualHost->GetWxObject(GetParent()));
	if (toolbar) toolbar->DestroyTool(GetControlID());
}

bool CValueToolBarSeparator::CanDeleteControl() const
{
	return m_parent->GetChildCount() > 1;
}

//**********************************************************************************
//*                                  Data	                                       *
//**********************************************************************************

bool CValueToolBarItem::LoadData(CMemoryReader& reader)
{
	m_propertyTitle->LoadData(reader);
	m_propertyPicture->LoadData(reader);
	m_propertyRepresentation->LoadData(reader);
	m_propertyContextMenu->LoadData(reader);
	m_properyTooltip->LoadData(reader);
	m_propertyEnabled->LoadData(reader);
	m_eventAction->LoadData(reader);

	//events 
	m_eventAction->LoadData(reader);
	return IValueControl::LoadData(reader);
}

bool CValueToolBarItem::SaveData(CMemoryWriter& writer)
{
	m_propertyTitle->SaveData(writer);
	m_propertyPicture->SaveData(writer);
	m_propertyRepresentation->SaveData(writer);
	m_propertyContextMenu->SaveData(writer);
	m_properyTooltip->SaveData(writer);
	m_propertyEnabled->SaveData(writer);
	m_eventAction->SaveData(writer);

	//events 
	m_eventAction->SaveData(writer);
	return IValueControl::SaveData(writer);
}

bool CValueToolBarSeparator::LoadData(CMemoryReader& reader)
{
	return IValueControl::LoadData(reader);
}

bool CValueToolBarSeparator::SaveData(CMemoryWriter& writer)
{
	return IValueControl::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

S_CONTROL_TYPE_REGISTER(CValueToolBarItem, "tool", "tool", g_controlToolBarItemCLSID);
S_CONTROL_TYPE_REGISTER(CValueToolBarSeparator, "toolSeparator", "tool", g_controlToolBarSeparatorCLSID);