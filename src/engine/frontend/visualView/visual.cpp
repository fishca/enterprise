////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor 
////////////////////////////////////////////////////////////////////////////

#include "visual.h"

#include "ctrl/control.h"
#include "ctrl/form.h"

#include "pageWindow.h"

#include <wx/collpane.h>

wxIMPLEMENT_ABSTRACT_CLASS(IVisualHost, wxScrolledWindow)

bool IVisualHost::CreateVisualHost()
{
	const CValueForm* valueForm = GetValueForm();

#if !defined(__WXGTK__ )
	GetParentBackgroundWindow()->Freeze();   // Prevent flickering on wx 2.8,
	// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
#endif

	if (valueForm != nullptr && IsShownHost()) {

		// --- [1] Show form 
		GetParentBackgroundWindow()->Show(true);

		// --- [2] Set the color of the form -------------------------------
		GetBackgroundWindow()->SetForegroundColour(valueForm->GetForegroundColour());
		GetBackgroundWindow()->SetBackgroundColour(valueForm->GetBackgroundColour());

		// --- [3] Title bar Setup
		SetCaption(valueForm->GetCaption());

		// --- [4] Default sizer Setup 
		SetOrientation(valueForm->GetOrient());

		// --- [5] Create the components of the form -------------------------
		// Used to save valueForm objects for later display

		for (unsigned int i = 0; i < valueForm->GetChildCount(); i++) {
			IValueFrame* child = valueForm->GetChild(i);
			// Recursively generate the ObjectTree
			try {
				// we have to put the content valueForm panel as parentObject in order
				// to SetSizeHints be called.
				GenerateControl(child, GetBackgroundWindow(), GetFrameSizer());
			}
			catch (std::exception& ex) {
				wxLogError(ex.what());
			}
		}
	}
	else {
		// There is no form to display
		GetParentBackgroundWindow()->Show(false);
	}

#if !defined(__WXGTK__)
	GetParentBackgroundWindow()->Thaw();
#endif

	return true;
}

bool IVisualHost::UpdateVisualHost()
{
	const CValueForm* valueForm = GetValueForm();

#if !defined(__WXGTK__ )
	GetParentBackgroundWindow()->Freeze();   // Prevent flickering on wx 2.8,
	// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
#endif

	if (valueForm != nullptr && IsShownHost()) {

		// --- [1] Show form 
		GetParentBackgroundWindow()->Show(true);

		// --- [2] Set the color of the form -------------------------------
		GetBackgroundWindow()->SetForegroundColour(valueForm->GetForegroundColour());
		GetBackgroundWindow()->SetBackgroundColour(valueForm->GetBackgroundColour());

		// --- [3] Title bar Setup
		SetCaption(valueForm->GetCaption());

		// --- [4] Default sizer Setup 
		SetOrientation(valueForm->GetOrient());

		// --- [5] Update the components of the form -------------------------
		// Used to save valueForm objects for later display
		for (unsigned int i = 0; i < valueForm->GetChildCount(); i++) {

			IValueFrame* child = valueForm->GetChild(i);

			// Recursively generate the ObjectTree
			try {
				// we have to put the content valueForm panel as parentObject in order
				// to SetSizeHints be called.
				RefreshControl(child, GetBackgroundWindow(), GetFrameSizer(), true);
			}
			catch (std::exception& ex) {
				wxLogError(ex.what());
			}
		}

		// --- [6] Calculate label size 
		CalculateLabelSize();

		// --- [7] Set sizer properties
		GetParentBackgroundWindow()->Layout();

		if (GetParentBackgroundWindow() != this) {
			GetParentBackgroundWindow()->GetSizer()->Fit(GetParentBackgroundWindow());
			GetParentBackgroundWindow()->SetClientSize(GetParentBackgroundWindow()->GetBestSize());
		}
		
		// --- [8] Refresh main window
		GetParentBackgroundWindow()->Refresh();
	}
	else {
		// There is no form to display
		GetParentBackgroundWindow()->Show(false);
		GetParentBackgroundWindow()->Refresh();
	}

#if !defined(__WXGTK__)
	GetParentBackgroundWindow()->Thaw();
#endif

	UpdateVirtualSize();
	return true;
}

bool IVisualHost::ClearVisualHost()
{
	const CValueForm* valueForm = GetValueForm();
	wxASSERT(valueForm);

	for (unsigned int i = 0; i < valueForm->GetChildCount(); i++) {
		IValueFrame* objChild = valueForm->GetChild(i);
		DeleteRecursive(objChild, true);
	}

	if (GetParentBackgroundWindow() != this) {
		GetParentBackgroundWindow()->DestroyChildren();
		GetParentBackgroundWindow()->SetSizer(nullptr); // *!*
	}

	return true;
}

IValueFrame* IVisualHost::GetObjectBase(wxObject* wxobject) const
{
	if (nullptr == wxobject) {
		wxLogError(_("wxObject was nullptr!"));
		return nullptr;
	}

	auto obj = m_wxObjects.find(wxobject);
	if (obj != m_wxObjects.end()) {
		return obj->second;
	}
	else {
		wxLogError(_("No corresponding IValueFrame for wxObject. Name: %s"), wxobject->GetClassInfo()->GetClassName());
		return nullptr;
	}
}

wxObject* IVisualHost::GetWxObject(IValueFrame* baseobject) const
{
	if (baseobject == nullptr) {
		wxLogError(_("baseobject was nullptr!"));
		return nullptr;
	}

	if (baseobject->GetComponentType() == COMPONENT_TYPE_FRAME)
		return GetFrameSizer();

	auto obj = m_baseObjects.find(baseobject);
	if (obj != m_baseObjects.end()) {
		return obj->second;
	}
	else {
		wxLogError(_("No corresponding wxObject for IValueFrame. Name: %s"), baseobject->GetClassName().c_str());
		return nullptr;
	}
}

void IVisualHost::DeleteRecursive(IValueFrame* control, bool force)
{
	for (unsigned int i = 0; i < control->GetChildCount(); i++) {
		DeleteRecursive(control->GetChild(i), force);
	}

	if (control->GetComponentType() == COMPONENT_TYPE_WINDOW) {
		wxWindow* controlWnd =
			dynamic_cast<wxWindow*>(GetWxObject(control));
		if (controlWnd != nullptr) {
			wxWindow* controlParent = controlWnd->GetParent();
			Cleanup(control, controlWnd);
			m_baseObjects.erase(control);
			m_wxObjects.erase(controlWnd);
			controlWnd->DeletePendingEvents(); /*!!!*/
			controlWnd->DestroyChildren();
			controlWnd->Destroy();
			if (!force && controlParent != nullptr) {
				controlParent->Layout();
			}
		}
	}
	else if (control->GetComponentType() == COMPONENT_TYPE_SIZER) {
		wxSizer* controlSizer =
			dynamic_cast<wxSizer*>(GetWxObject(control));

		if (controlSizer != nullptr) {
			IValueFrame* controlParent = control->GetParent();
			Cleanup(control, controlSizer);
			m_baseObjects.erase(control);
			m_wxObjects.erase(controlSizer);
			if (controlParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				controlParent = controlParent->GetParent();
			}
			wxSizer* controlParentSizer = nullptr;
			if (controlParent->GetClassName() == wxT("notebookPage")) {
				CPanelPage* pageWnd = dynamic_cast<CPanelPage*>(GetWxObject(controlParent));
				wxASSERT(pageWnd); controlParentSizer = pageWnd->GetSizer();
			}
			else {
				controlParentSizer =
					dynamic_cast<wxSizer*>(GetWxObject(controlParent));
			}
			if (controlParentSizer != nullptr) {
				controlParentSizer->Detach(controlSizer);
			}
			delete controlSizer;
			if (!force && controlParentSizer != nullptr) {
				controlParentSizer->Layout();
			}
		}
	}
	else {
		wxObject* controlObj = GetWxObject(control);
		if (controlObj != nullptr) {
			Cleanup(control, controlObj);
			m_baseObjects.erase(control);
			m_wxObjects.erase(controlObj);
			delete controlObj;
		}
	}
}

void IVisualHost::CreateControl(IValueFrame* obj, IValueFrame* parent, bool firstCreated)
{
	IValueFrame* objControl = obj; IValueFrame* objParent = parent ? parent : obj->GetParent();
	wxWindow* windowObj = nullptr; wxObject* parentObj = nullptr; wxWindow* hostWnd = GetBackgroundWindow();

	if (objParent && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		objControl = objParent; parentObj = GetWxObject(objParent->GetParent());
		objParent = objParent->GetParent();
	}
	else {
		parentObj = GetWxObject(objParent);
	}

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer")) {
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		wxASSERT(staticBoxSizer); windowObj = staticBoxSizer->GetStaticBox();
	}
	else {
		windowObj = dynamic_cast<wxWindow*>(parentObj);
	}

	IValueFrame* nextParent = objParent;
	while (!windowObj && nextParent) {
		if (nextParent->GetComponentType() == COMPONENT_TYPE_WINDOW) {
			windowObj =
				dynamic_cast<wxWindow*>(GetWxObject(nextParent));
			break;
		}
		nextParent = nextParent->GetParent();
	}

	if (windowObj == nullptr)
		windowObj = hostWnd;

#if !defined(__WXGTK__ )
	if (IsShown()) {
		Freeze();   // Prevent flickering on wx 2.8,
		// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
	}
#endif

	//first create elements 
	GenerateControl(objControl, windowObj, parentObj, firstCreated);

	//and update it 
	RefreshControl(objControl, windowObj, parentObj);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer")) {
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	windowObj->Layout();

	wxWindow* windowParent = GetParentBackgroundWindow();
	wxASSERT(windowParent);

	windowParent->GetSizer()->Fit(windowParent);
	windowParent->SetClientSize(windowParent->GetBestSize());

	windowObj->Refresh();
	windowParent->Refresh();

#if !defined(__WXGTK__)
	Thaw();
#endif

	UpdateVirtualSize();
}

void IVisualHost::UpdateControl(IValueFrame* obj, IValueFrame* parent)
{
	IValueFrame* objControl = obj; IValueFrame* objParent = parent ? parent : obj->GetParent();
	wxWindow* windowObj = nullptr; wxObject* parentObj = nullptr; wxWindow* hostWnd = GetBackgroundWindow();

	if (objParent && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
	{
		objControl = objParent; parentObj = GetWxObject(objParent->GetParent());
		objParent = objParent->GetParent();
	}
	else
	{
		parentObj = GetWxObject(objParent);
	}

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		wxASSERT(staticBoxSizer); windowObj = staticBoxSizer->GetStaticBox();
	}
	else
	{
		windowObj = dynamic_cast<wxWindow*>(parentObj);
	}

	IValueFrame* nextParent = objParent;
	while (!windowObj && nextParent)
	{
		if (nextParent->GetComponentType() == COMPONENT_TYPE_WINDOW) {
			windowObj = dynamic_cast<wxWindow*>(GetWxObject(nextParent));
			break;
		}
		nextParent = nextParent->GetParent();
	}

	if (!windowObj) windowObj = hostWnd;

#if !defined(__WXGTK__ )
	if (IsShown())
	{
		Freeze();   // Prevent flickering on wx 2.8,
		// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
	}
#endif

	RefreshControl(objControl, windowObj, parentObj);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	windowObj->Layout();

	wxWindow* windowParent = GetParentBackgroundWindow();
	wxASSERT(windowParent);

	windowParent->GetSizer()->Fit(windowParent);
	windowParent->SetClientSize(windowParent->GetBestSize());

	windowObj->Refresh();
	windowParent->Refresh();

#if !defined(__WXGTK__)
	Thaw();
#endif

	UpdateVirtualSize();
}

void IVisualHost::RemoveControl(IValueFrame* obj, IValueFrame* parent)
{
	IValueFrame* objControl = obj; IValueFrame* objParent = parent ? parent : obj->GetParent();
	wxWindow* windowObj = nullptr; wxObject* parentObj = nullptr; wxWindow* hostWnd = GetBackgroundWindow();

	if (objParent && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
	{
		objControl = objParent; parentObj = GetWxObject(objParent->GetParent());
		objParent = objParent->GetParent();
	}
	else
	{
		parentObj = GetWxObject(objParent);
	}

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		wxASSERT(staticBoxSizer); windowObj = staticBoxSizer->GetStaticBox();
	}
	else
	{
		windowObj = dynamic_cast<wxWindow*>(parentObj);
	}

	IValueFrame* nextParent = objParent;
	while (!windowObj && nextParent)
	{
		if (nextParent->GetComponentType() == COMPONENT_TYPE_WINDOW) {
			windowObj = dynamic_cast<wxWindow*>(GetWxObject(nextParent));
			break;
		}
		nextParent = nextParent->GetParent();
	}

	if (!windowObj) windowObj = hostWnd;

#if !defined(__WXGTK__ )
	if (IsShown())
	{
		Freeze();   // Prevent flickering on wx 2.8,
		// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
	}
#endif

	DeleteRecursive(objControl);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	windowObj->Layout();

	wxWindow* windowParent = GetParentBackgroundWindow();
	wxASSERT(windowParent);

	windowParent->GetSizer()->Fit(windowParent);
	windowParent->SetClientSize(windowParent->GetBestSize());

	windowObj->Refresh();
	windowParent->Refresh();

#if !defined(__WXGTK__)
	Thaw();
#endif

	UpdateVirtualSize();
}

void IVisualHost::GenerateControl(IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject, bool firstCreated)
{
	class CControlCreator
	{
	public:
		static void CreateControl(IVisualHost* visualHost, IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject, bool firstCreated)
		{
			// Create Object
			wxObject* createdObject = visualHost->Create(obj, wxparent);
			wxWindow* createdWindow = nullptr;
			wxSizer* createdSizer = nullptr;

			wxWindow* parentObj = wxparent;

			switch (obj->GetComponentType())
			{
			case COMPONENT_TYPE_WINDOW:
			{
				if (obj->GetClassName() == wxT("notebookPage")) {
					CPanelPage* pageWindow = wxDynamicCast(createdObject, CPanelPage);
					if (pageWindow) {
						createdWindow = pageWindow;
						createdSizer = pageWindow->GetSizer();

						parentObj = pageWindow;
					}
				}
				else {
					createdWindow = wxDynamicCast(createdObject, wxWindow);
				}
				break;
			}
			case COMPONENT_TYPE_SIZER:
			case COMPONENT_TYPE_SIZERITEM:
			{
				if (obj->GetClassName() == wxT("staticboxsizer")) {
					wxStaticBoxSizer* staticBoxSizer = wxDynamicCast(createdObject, wxStaticBoxSizer);
					if (staticBoxSizer) {
						createdWindow = staticBoxSizer->GetStaticBox();
						createdSizer = staticBoxSizer;
					}
				}
				else
				{
					createdSizer = wxDynamicCast(createdObject, wxSizer);
				}
				break;
			}
			default: break;
			}

			// Associate the wxObject* with the IValueFrame*
			visualHost->m_baseObjects.insert_or_assign(obj, createdObject);
			visualHost->m_wxObjects.insert_or_assign(createdObject, obj);

			// Access to collapsible pane
			wxCollapsiblePane* collpane = wxDynamicCast(createdObject, wxCollapsiblePane);
			if (collpane != nullptr) {
				createdWindow = collpane->GetPane();
				createdObject = createdWindow;
			}

			// New wxparent for the window's children
			wxWindow* new_wxparent = (createdWindow ? createdWindow : wxparent);

			// Recursively generate the children
			for (unsigned int i = 0; i < obj->GetChildCount(); i++) {
				CreateControl(visualHost, obj->GetChild(i), new_wxparent, createdObject, firstCreated);
			}

			//if new obj then call on created 
			visualHost->OnCreated(obj, createdObject, wxparent, firstCreated);
		}
	};

	CControlCreator::CreateControl(this, obj, wxparent, parentObject, firstCreated);
}

void IVisualHost::RefreshControl(IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject, bool refreshForm)
{
	class CControlUpdater
	{
	public:
		static void UpdateControl(IVisualHost* visualHost, IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject)
		{
			// Create Object
			wxObject* createdObject = visualHost->GetWxObject(obj);
			wxWindow* createdWindow = nullptr;
			wxSizer* createdSizer = nullptr;

			wxWindow* parentObj = wxparent;

			switch (obj->GetComponentType())
			{
			case COMPONENT_TYPE_WINDOW:
			{
				if (obj->GetClassName() == wxT("notebookPage")) {
					CPanelPage* pageWindow = wxDynamicCast(createdObject, CPanelPage);
					if (pageWindow)
					{
						createdWindow = pageWindow;
						createdSizer = pageWindow->GetSizer();

						parentObj = pageWindow;
					}
				}
				else {
					createdWindow = wxDynamicCast(createdObject, wxWindow);
				}

				break;
			}
			case COMPONENT_TYPE_SIZER:
			case COMPONENT_TYPE_SIZERITEM:
			{
				if (obj->GetClassName() == wxT("staticboxsizer")) {
					wxStaticBoxSizer* staticBoxSizer = wxDynamicCast(createdObject, wxStaticBoxSizer);
					if (staticBoxSizer) {
						createdWindow = staticBoxSizer->GetStaticBox();
						createdSizer = staticBoxSizer;
					}
				}
				else {
					createdSizer = wxDynamicCast(createdObject, wxSizer);
				} break;
			}
			default: break;
			}

			visualHost->Update(obj, createdObject);

			// Access to collapsible pane
			wxCollapsiblePane* collpane = wxDynamicCast(createdObject, wxCollapsiblePane);
			if (collpane != nullptr) {
				createdWindow = collpane->GetPane();
				createdObject = createdWindow;
			}

			// New wxparent for the window's children
			wxWindow* new_wxparent = (createdWindow ? createdWindow : wxparent);

			// Recursively generate the children
			for (unsigned int i = 0; i < obj->GetChildCount(); i++) {
				UpdateControl(visualHost, obj->GetChild(i), new_wxparent, createdObject);
			}

			//if new obj then call on created 
			visualHost->OnUpdated(obj, createdObject, wxparent);

			// If the created object is a sizer and the parent object is a window, set the sizer to the window
			if (
				(createdSizer != nullptr && nullptr != wxDynamicCast(parentObject, wxWindow))
				||
				(nullptr == parentObject && createdSizer != nullptr)
				)
			{
				parentObj->SetSizer(createdSizer);

				if (parentObject)
					createdSizer->SetSizeHints(parentObj);

				parentObj->SetAutoLayout(true);
				parentObj->Layout();
			}
		}
	};

	CControlUpdater::UpdateControl(this, obj, wxparent, parentObject);

	if (!refreshForm) CalculateLabelSize(obj);
}

#include "frontend/win/ctrls/dynamicBorder.h"

bool IVisualHost::CalculateLabelSize(IValueFrame* control)
{
	const CValueForm* valueForm = GetValueForm();
	if (valueForm == nullptr) return false;

	class CCalculateSize
	{
		static void Calculate(IValueFrame* child, wxBoxSizer* parentSizer, int& maxX) {
			if (child->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				child = child->GetChild(0);
			}
			wxASSERT(child);

			wxObject* wx_object = child->GetWxObject();

			wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(wx_object);
			IDynamicBorder* childCtrl = dynamic_cast<IDynamicBorder*>(wx_object);

			if (childCtrl != nullptr && childCtrl->AllowCalc()) {
				wxStaticText* childStaticText = childCtrl->GetStaticText();
				const wxString& strLabel = childStaticText->GetLabelText();
				if (!strLabel.IsEmpty()) {
					const wxSize& childSize = childStaticText->GetTextExtent(strLabel);
					if (childSize.GetX() > maxX) {
						maxX = childSize.GetX();
					}
				}
			}

			if (parentSizer->GetOrientation() == wxOrientation::wxHORIZONTAL) {
				return;
			}

			for (unsigned int idx = 0; idx < child->GetChildCount(); idx++) {
				Calculate(child->GetChild(idx), sizer ? sizer : parentSizer, maxX);
			}
		};

		static void Apply(IValueFrame* child, wxBoxSizer* parentSizer, int& maxX) {
			IValueFrame* childParent = child->GetParent();
			if (child->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				child = child->GetChild(0);
			}
			wxASSERT(child);

			wxObject* wx_object = child->GetWxObject();

			wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(wx_object);
			IDynamicBorder* childCtrl = dynamic_cast<IDynamicBorder*>(wx_object);

			int currMax = maxX;

			if (maxX != wxNOT_FOUND ||
				parentSizer->GetOrientation() == wxOrientation::wxVERTICAL) {
				Calculate(childParent, sizer ? sizer : parentSizer, maxX);
			}

			if (childCtrl != nullptr && childCtrl->AllowCalc()) {
				childCtrl->BeforeCalc();
			}

			for (unsigned int idx = 0; idx < child->GetChildCount(); idx++) {
				Apply(child->GetChild(idx), sizer ? sizer : parentSizer, currMax);
			}

			if (childCtrl != nullptr && childCtrl->AllowCalc()) {
				wxStaticText* childStaticText = childCtrl->GetStaticText();
				if (maxX != wxNOT_FOUND) {
					childStaticText->SetMinSize(wxSize(maxX, wxNOT_FOUND));
				}
				else {
					childStaticText->SetMinSize(wxDefaultSize);
				}
				if (parentSizer->GetOrientation() == wxOrientation::wxHORIZONTAL) {
					maxX = wxNOT_FOUND;
				}
				childCtrl->AfterCalc();
			}
		};

	public:

		static bool CalculateAndApply(const CValueForm* valueForm, IDynamicBorder* control = nullptr)
		{
			if (control != nullptr && control->AllowCalc()) {
				control->BeforeCalc();
			}

			wxBoxSizer* parentSizer = dynamic_cast<wxBoxSizer*>(valueForm->GetWxObject());
			int currMaxX = 0;
			for (unsigned int idx = 0; idx < valueForm->GetChildCount(); idx++) {
				Apply(valueForm->GetChild(idx), parentSizer, currMaxX);
			}

			if (control != nullptr && control->AllowCalc()) {
				control->AfterCalc();
			}

			return true;
		}
	};

	return CCalculateSize::CalculateAndApply(valueForm,
		dynamic_cast<IDynamicBorder*>(control));
}

void IVisualHost::UpdateVirtualSize()
{
	int w, h, panelW, panelH;
	GetVirtualSize(&w, &h);
	GetBackgroundWindow()->GetSize(&panelW, &panelH);
	panelW += 20; panelH += 20;
	if (panelW != w || panelH != h)
		SetVirtualSize(panelW, panelH);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<IVisualEditorNotebook*> IVisualEditorNotebook::sm_visualEditor = {};

/////////////////////////////////////////////////////////////////////////////////////////////////////

IVisualEditorNotebook* IVisualEditorNotebook::FindEditorByForm(IValueFrame* valueForm)
{
	auto& it = std::find_if(sm_visualEditor.begin(), sm_visualEditor.end(),
		[valueForm](IVisualEditorNotebook* visualNotebook) {
			return valueForm == visualNotebook->GetValueForm();
		}
	);

	if (it != sm_visualEditor.end())
		return *it;

	return nullptr;
}

void IVisualEditorNotebook::CreateVisualEditor()
{
	sm_visualEditor.push_back(this);
}

void IVisualEditorNotebook::DestroyVisualEditor()
{
	auto& it = std::find(sm_visualEditor.begin(), sm_visualEditor.end(), this);
	if (it != sm_visualEditor.end()) {
		sm_visualEditor.erase(it);
	}
}
