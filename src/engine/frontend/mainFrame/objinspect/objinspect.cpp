#include "objinspect.h"

enum {
	WXOES_PROPERTY_GRID = wxID_HIGHEST + 1000
};

// -----------------------------------------------------------------------
// CObjectInspector
// -----------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(CObjectInspector, wxPanel)
EVT_PG_CHANGING(WXOES_PROPERTY_GRID, CObjectInspector::OnPropertyGridChanging)
EVT_PG_CHANGED(WXOES_PROPERTY_GRID, CObjectInspector::OnPropertyGridChanged)
EVT_PG_ITEM_COLLAPSED(WXOES_PROPERTY_GRID, CObjectInspector::OnPropertyGridExpand)
EVT_PG_ITEM_EXPANDED(WXOES_PROPERTY_GRID, CObjectInspector::OnPropertyGridExpand)
EVT_PG_SELECTED(WXOES_PROPERTY_GRID, CObjectInspector::OnPropertyGridItemSelected)
EVT_CHILD_FOCUS(CObjectInspector::OnChildFocus)
wxEND_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////
// CObjectInspector
///////////////////////////////////////////////////////////////////////////////

CObjectInspector::CObjectInspector(wxWindow* parent, int id, int style)
	: wxPanel(parent, id), m_style(style), m_currentSel(nullptr)
{
	m_pg = CreatePropertyGridManager(this, WXOES_PROPERTY_GRID);

	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
	topSizer->Add(m_pg, 1, wxALL | wxEXPAND, 0);
	SetSizer(topSizer);

	CObjectInspector::Connect(wxID_ANY, wxEVT_OES_PROP_PICTURE_CHANGED, wxCommandEventHandler(CObjectInspector::OnBitmapPropertyChanged));
}

CObjectInspector::~CObjectInspector()
{
	CObjectInspector::Disconnect(wxID_ANY, wxEVT_OES_PROP_PICTURE_CHANGED, wxCommandEventHandler(CObjectInspector::OnBitmapPropertyChanged));
}

///////////////////////////////////////////////////////////////////////////////

void CObjectInspector::SavePosition()
{
	// Save Layout
	wxConfigBase* config = wxConfigBase::Get();
	config->Write(wxT("/mainFrame/objectInspector/DescBoxHeight"), m_pg->GetDescBoxHeight());
}

#include "frontend/mainFrame/mainFrame.h"

CObjectInspector* CObjectInspector::GetObjectInspector()
{
	return CDocMDIFrame::GetObjectInspector();
}

#include "frontend/visualView/formdefs.h"

void CObjectInspector::Create(bool force)
{
	IPropertyObject* sel_obj = GetSelectedObject();

	if (sel_obj && (sel_obj != m_currentSel || force)) {

		m_pg->Freeze();
		m_currentSel = sel_obj;

		int pageNumber = m_pg->GetSelectedPage();

		wxString pageName;
		if (pageNumber != wxNOT_FOUND) {
			pageName = m_pg->GetPageName(pageNumber);
		}

		// Clear Property Grid Manager
		m_pg->Clear();

		m_propMap.clear();
		m_eventMap.clear();

		std::map<wxString, IProperty*> propMap, dummyPropMap;
		std::map<wxString, IEvent*> eventMap, dummyEventMap;

		// We create the categories with the properties of the object organized by "classes"
		CreateCategory(sel_obj->GetClassName(), sel_obj, propMap, false);

		IPropertyObject* owner = sel_obj->GetOwner();

		if (owner != nullptr) {
			if (owner->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				CreateCategory(sel_obj->GetClassName(), owner, dummyPropMap, false);
			}
		}

		CreateCategory(sel_obj->GetClassName(), sel_obj, eventMap, true);

		if (owner != nullptr) {
			if (owner->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				CreateCategory(sel_obj->GetClassName(), owner, dummyEventMap, true);
			}
		}

		// Select previously selected page, or first page
		if (m_pg->GetPageCount() > 0)
		{
			int pageIndex = m_pg->GetPageByName(pageName);
			if (wxNOT_FOUND != pageIndex) {
				m_pg->SelectPage(pageIndex);
			}
			else {
				m_pg->SelectPage(0);
			}
		}

		m_currentSel->OnPropertyCreated();

		m_pg->Refresh();
		m_pg->Update();

		m_pg->Thaw();
	}

	if (m_currentSel != nullptr) {
		m_pg->Freeze();
		for (auto& prop : m_propMap)
			m_currentSel->OnPropertyRefresh(m_pg, prop.first, prop.second);
		for (auto event : m_eventMap)
			m_currentSel->OnEventRefresh(m_pg, event.first, event.second);
		for (auto prop : m_propMap) {
			wxPGProperty* property = prop.first;
			if (property != nullptr) {
				wxPGProperty* parentProperty = property->GetParent();
				if (parentProperty->IsCategory() &&
					parentProperty->IsVisible() != (!property->HasFlag(wxPG_PROP_HIDDEN))) {
					bool visible = false;
					for (unsigned int idx = 0; idx < parentProperty->GetChildCount(); idx++) {
						wxPGProperty* currChild = parentProperty->Item(idx);
						wxASSERT(currChild);
						if (!currChild->HasFlag(wxPG_PROP_HIDDEN))
							visible = true;
					}
					if (parentProperty->IsVisible() != visible)
						parentProperty->Hide(!visible);
				}
			}
		}
		m_pg->Thaw();
	}

	RestoreLastSelectedPropItem();
}

bool CObjectInspector::IsShownProperty() const
{
	if (mainFrame != nullptr)
		return mainFrame->IsShownProperty();
	return false;
}

void CObjectInspector::ShowProperty()
{
	if (mainFrame != nullptr)
		mainFrame->ShowProperty();
}

void CObjectInspector::ClearProperty()
{
	Freeze();

	m_currentSel = nullptr;

	// Clear Property Grid Manager
	m_pg->Clear();

	m_propMap.clear();
	m_eventMap.clear();

	m_pg->Refresh();
	m_pg->Update();

	Thaw();
}

wxPropertyGridManager* CObjectInspector::CreatePropertyGridManager(wxWindow* parent, wxWindowID id)
{
	int pgStyle;
	int defaultDescBoxHeight;

	switch (m_style) {
	case wxOES_OI_MULTIPAGE_STYLE:
		pgStyle = wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLBAR | wxPG_DESCRIPTION | wxPG_TOOLTIPS | wxPGMAN_DEFAULT_STYLE;
		defaultDescBoxHeight = 50;
		break;
	case wxOES_OI_DEFAULT_STYLE:
	case wxOES_OI_SINGLE_PAGE_STYLE:
	default:
		pgStyle = wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_DESCRIPTION | wxPG_TOOLTIPS | wxPGMAN_DEFAULT_STYLE;
		defaultDescBoxHeight = 150;
		break;
	}

	int descBoxHeight;
	wxConfigBase* config = wxConfigBase::Get();
	config->Read(wxT("/mainFrame/objectInspector/DescBoxHeight"), &descBoxHeight, defaultDescBoxHeight);
	if (descBoxHeight == wxNOT_FOUND) {
		descBoxHeight = defaultDescBoxHeight;
	}

	wxPropertyGridManager* pg = new wxPropertyGridManager(parent, id, wxDefaultPosition, wxDefaultSize, pgStyle);	
	pg->SetExtraStyle(wxPG_EX_NATIVE_DOUBLE_BUFFERING);
	pg->SetDescBoxHeight(descBoxHeight);
	pg->SendSizeEvent();
	return pg;
}

wxPGProperty* CObjectInspector::GetProperty(IProperty* prop)
{
	wxPGProperty* result = prop->GetPGProperty();
	if (result != nullptr) {
		result->SetHelpString(prop->GetHelp());
		result->Enable(prop->IsEditable());
	}
	return result;
}

wxPGProperty* CObjectInspector::GetEvent(IEvent* event)
{
	wxPGProperty* result = event->GetPGProperty();
	if (result != nullptr) {
		result->SetHelpString(event->GetHelp());
		result->Enable(event->IsEditable());
	}
	return result;
}

bool CObjectInspector::ModifyProperty(IProperty* prop, const wxVariant& newValue)
{
	const wxVariant oldValue = prop->GetValue();
	if (m_currentSel->OnPropertyChanging(prop, newValue)) {
		prop->SetValue(newValue);
		m_currentSel->OnPropertyChanged(prop, oldValue, newValue);
		return true;
	}
	return false;
}

bool CObjectInspector::ModifyEvent(IEvent* event, const wxVariant& newValue)
{
	const wxVariant oldValue = event->GetValue();
	if (m_currentSel->OnEventChanging(event, newValue)) {
		event->SetValue(newValue);
		m_currentSel->OnEventChanged(event, oldValue, newValue);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

void CObjectInspector::OnPropertyGridChanging(wxPropertyGridEvent& event)
{
	wxPGProperty* propPtr = event.GetProperty();
	std::map< wxPGProperty*, IProperty*>::iterator itProperty = m_propMap.find(propPtr);
	if (itProperty != m_propMap.end()) {
		IProperty* prop_ptr = itProperty->second;
		// Update displayed description for the new selection
		const wxString& helpString = prop_ptr->GetHelp();
		if (!ModifyProperty(prop_ptr, event.GetPropertyValue()))
			event.Veto();
		const wxString& localized = wxGetTranslation(helpString);
		m_pg->SetPropertyHelpString(propPtr, localized);
		m_pg->SetDescription(propPtr->GetLabel(), localized);
		return;
	}

	std::map< wxPGProperty*, IEvent*>::iterator itEvent = m_eventMap.find(propPtr);
	if (itEvent != m_eventMap.end()) {
		IEvent* event_ptr = itEvent->second;
		// Update displayed description for the new selection
		const wxString& helpString = event_ptr->GetHelp();
		if (!ModifyEvent(event_ptr, event.GetPropertyValue()))
			event.Veto();
		const wxString& localized = wxGetTranslation(helpString);
		m_pg->SetPropertyHelpString(propPtr, localized);
		m_pg->SetDescription(propPtr->GetLabel(), localized);
		return;
	}
}

void CObjectInspector::OnPropertyGridChanged(wxPropertyGridEvent& event)
{
	if (m_currentSel != nullptr) {
		m_pg->Freeze();
		for (auto prop : m_propMap) m_currentSel->OnPropertyRefresh(m_pg, prop.first, prop.second);
		for (auto event : m_eventMap) m_currentSel->OnEventRefresh(m_pg, event.first, event.second);
		for (auto prop : m_propMap) {
			wxPGProperty* property = prop.first;
			if (property != nullptr) {
				wxPGProperty* parentProperty = property->GetParent();
				if (parentProperty->IsCategory() &&
					parentProperty->IsVisible() != (!property->HasFlag(wxPG_PROP_HIDDEN))) {
					bool visible = false;
					for (unsigned int idx = 0; idx < parentProperty->GetChildCount(); idx++) {
						wxPGProperty* currChild = parentProperty->Item(idx);
						wxASSERT(currChild);
						if (!currChild->HasFlag(wxPG_PROP_HIDDEN)) visible = true;
					}
					if (parentProperty->IsVisible() != visible) parentProperty->Hide(!visible);
				}
			}
			IProperty* prop_ptr = prop.second;
			wxASSERT(prop_ptr);
			prop_ptr->RefreshPGProperty(property);
		}

		for (auto evt : m_eventMap) {
			IEvent* event_ptr = evt.second;
			wxASSERT(event_ptr);
			event_ptr->RefreshPGProperty(evt.first);
		};

		m_pg->Thaw();
	}
	event.Skip();
}

void CObjectInspector::OnPropertyGridExpand(wxPropertyGridEvent& event)
{
	m_isExpanded[event.GetPropertyName()] = event.GetProperty()->IsExpanded();

	wxPGProperty* egProp = m_pg->GetProperty(event.GetProperty()->GetName());
	if (egProp != nullptr) {
		if (event.GetProperty()->IsExpanded()) {
			m_pg->Expand(egProp);
		}
		else {
			m_pg->Collapse(egProp);
		}
	}
}

void CObjectInspector::OnPropertyGridItemSelected(wxPropertyGridEvent& event)
{
	wxPGProperty* propPtr = event.GetProperty();
	if (propPtr != nullptr) {
		m_strSelPropItem = m_pg->GetPropertyName(propPtr);
		std::map< wxPGProperty*, IProperty*>::iterator it = m_propMap.find(propPtr);
		if (m_propMap.end() == it) {
			// Could be a child property
			propPtr = propPtr->GetParent();
			it = m_propMap.find(propPtr);
		}
		if (m_currentSel && it != m_propMap.end()) {
			m_currentSel->OnPropertySelected(it->second);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void CObjectInspector::OnBitmapPropertyChanged(wxCommandEvent& event)
{
	wxLogDebug(wxT("OI::BitmapPropertyChanged: %s"), event.GetString().c_str());

	const wxString strPropName = event.GetString().BeforeFirst(':');
	wxString strPropVal = event.GetString().AfterFirst(':');

	//if (!propVal.IsEmpty()) {
	//	wxPGBitmapProperty* bp = wxDynamicCast(m_pg->GetPropertyByLabel(strPropName), wxPGBitmapProperty);
	//	if (bp != nullptr) {
	//		bp->UpdateChildValues(propVal);
	//	}
	//}
}

void CObjectInspector::OnChildFocus(wxChildFocusEvent&) {
	// do nothing to avoid "scrollbar jump" if wx2.9 is used
}