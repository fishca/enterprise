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

#if !defined(__WXGTK__ )
#define __FREEZE_CONTROL__
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool IVisualHost::CreateVisualHost()
{
	const CValueForm* valueForm = GetValueForm();

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Freeze();   // Prevent flickering on wx 2.8,
	// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
#endif

	if (valueForm != nullptr && IsShownHost()) {

		// --- [1] Create sizer
		GetBackgroundWindow()->SetSizer(new wxBoxSizer(wxVERTICAL));

		// --- [2] Show form 
		GetParentBackgroundWindow()->Show(true);

		// --- [3] Set the color of the form -------------------------------
		GetBackgroundWindow()->SetForegroundColour(valueForm->GetForegroundColour());
		GetBackgroundWindow()->SetBackgroundColour(valueForm->GetBackgroundColour());

		// --- [4] Title bar Setup
		SetCaption(valueForm->GetCaption());

		// --- [5] Default sizer Setup 
		SetOrientation(valueForm->GetOrient());

		// --- [6] Create the components of the form -------------------------
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

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif

	return true;
}

bool IVisualHost::UpdateVisualHost()
{
	const CValueForm* valueForm = GetValueForm();

#if defined(__FREEZE_CONTROL__)
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
				RefreshControl(child, GetBackgroundWindow(), GetFrameSizer());
			}
			catch (std::exception& ex) {
				wxLogError(ex.what());
			}
		}

		// --- [6] Calculate label size 
		CalculateLabelSize();

		// --- [7] Set sizer properties
		UpdateHostSize();
	}
	else {
		// There is no form to display
		GetParentBackgroundWindow()->Show(false);
		GetParentBackgroundWindow()->Refresh();
	}

	UpdateVirtualSize();

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif

	return true;
}

bool IVisualHost::ClearVisualHost()
{
#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Freeze();   // Prevent flickering on wx 2.8,
	// Causes problems on wx 2.9 in wxGTK (e.g. wxNoteBook objects)
#endif

	CValueForm* valueForm = GetValueForm();
	if (valueForm != nullptr) ClearControl(valueForm, true);

	GetParentBackgroundWindow()->DestroyChildren();
	GetParentBackgroundWindow()->SetSizer(nullptr); // *!*

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

IValueFrame* IVisualHost::GetObjectBase(const wxObject* wxobject) const
{
	if (nullptr == wxobject) {
		wxLogError(_("wxObject was nullptr!"));
		return nullptr;
	}

	for (auto& pair : m_baseObjects) {
		if (pair.second == wxobject)
			return pair.first;
	}

	wxLogError(_("No corresponding IValueFrame for wxObject. Name: %s"), wxobject->GetClassInfo()->GetClassName());
	return nullptr;
}

wxObject* IVisualHost::GetWxObject(const IValueFrame* baseobject) const
{
	if (baseobject == nullptr) {
		wxLogError(_("baseobject was nullptr!"));
		return nullptr;
	}

	if (baseobject->GetComponentType() == COMPONENT_TYPE_FRAME)
		return GetFrameSizer();

	for (auto& pair : m_baseObjects) {
		if (pair.first == baseobject)
			return pair.second;
	}

	wxLogError(_("No corresponding wxObject for IValueFrame. Name: %s"), baseobject->GetClassName().c_str());
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

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
			visualHost->AppendInnerControl(obj, createdObject);

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

void IVisualHost::RefreshControl(IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject)
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
				} break;
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
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

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

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Freeze();
#endif

	//first create elements 
	GenerateControl(objControl, windowObj, parentObj, firstCreated);

	//and update it 
	RefreshControl(objControl, windowObj, parentObj);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer")) {
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif
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

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Freeze();
#endif

	RefreshControl(objControl, windowObj, parentObj);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif
}

void IVisualHost::RemoveControl(IValueFrame* obj, IValueFrame* parent)
{
	IValueFrame* objControl = obj; IValueFrame* objParent = parent ? parent : obj->GetParent();
	wxObject* parentObj = nullptr;

	if (objParent && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
	{
		objControl = objParent; parentObj = GetWxObject(objParent->GetParent());
		objParent = objParent->GetParent();
	}
	else
	{
		parentObj = GetWxObject(objParent);
	}

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Freeze();
#endif

	ClearControl(objControl);

	if (objParent && objParent->GetClassName() == wxT("staticboxsizer"))
	{
		wxStaticBoxSizer* staticBoxSizer = dynamic_cast<wxStaticBoxSizer*>(parentObj);
		staticBoxSizer->Layout();
	}

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();

#if defined(__FREEZE_CONTROL__)
	GetParentBackgroundWindow()->Thaw();
#endif
}

#include "frontend/win/ctrls/dynamicBorder.h"

void IVisualHost::ClearControl(IValueFrame* control, bool force)
{
	class CControlCleaner {

		static inline void DeleteObject(IVisualHost* visualHost, IValueFrame* control, bool force) {

			if (control->GetComponentType() == COMPONENT_TYPE_WINDOW) {

				wxWindow* controlWnd =
					dynamic_cast<wxWindow*>(visualHost->GetWxObject(control));

				if (controlWnd != nullptr) {
					wxWindow* controlParent = controlWnd->GetParent();
					visualHost->Cleanup(control, controlWnd);
					visualHost->RemoveInnerControl(control);
					controlWnd->DeletePendingEvents(); /*!!!*/
					//controlWnd->DestroyChildren();
					controlWnd->Destroy();
					if (!force && controlParent != nullptr) {
						controlParent->Layout();
					}
				}
			}
			else if (control->GetComponentType() == COMPONENT_TYPE_SIZER) {

				wxSizer* controlSizer =
					dynamic_cast<wxSizer*>(visualHost->GetWxObject(control));

				if (controlSizer != nullptr) {
					IValueFrame* controlParent = control->GetParent();
					visualHost->Cleanup(control, controlSizer);
					visualHost->RemoveInnerControl(control);
					if (controlParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
						controlParent = controlParent->GetParent();
					}
					wxSizer* controlParentSizer = nullptr;
					if (controlParent->GetClassName() == wxT("notebookPage")) {
						CPanelPage* pageWnd = dynamic_cast<CPanelPage*>(visualHost->GetWxObject(controlParent));
						wxASSERT(pageWnd); controlParentSizer = pageWnd->GetSizer();
					}
					else {
						controlParentSizer =
							dynamic_cast<wxSizer*>(visualHost->GetWxObject(controlParent));
					}
					if (controlParentSizer != nullptr) {
						controlParentSizer->Detach(controlSizer);
					}
					wxDELETE(controlSizer);
					if (!force && controlParentSizer != nullptr) {
						controlParentSizer->Layout();
					}
				}
			}
			else if (control->GetComponentType() != COMPONENT_TYPE_FRAME) {
				wxObject* controlObj = visualHost->GetWxObject(control);
				if (controlObj != nullptr) {
					visualHost->Cleanup(control, controlObj);
					visualHost->RemoveInnerControl(control);
					wxDELETE(controlObj);
				}
			}
		}

	public:

		static inline void ClearControl(IVisualHost* visualHost, IValueFrame* control, bool force) {

			for (unsigned int i = 0; i < control->GetChildCount(); i++) {
				ClearControl(visualHost,
					control->GetChild(i), force
				);
			}

			DeleteObject(visualHost, control, force);
		}
	};

	CControlCleaner::ClearControl(this, control, force);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool IVisualHost::CalculateLabelSize(IValueFrame* control)
{
	static wxCoord widthTextMax, heightTextTotal;

	class CControlCalculateSize {

		static inline void Calculate(const IValueFrame* child, wxBoxSizer* parentSizer, int& maxX) {

			if (child->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				child = child->GetChild(0);
			}

			wxASSERT(child);

			wxObject* wx_object = child->GetWxObject();

			wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(wx_object);
			wxControlDynamicBorder* childCtrl = dynamic_cast<wxControlDynamicBorder*>(wx_object);

			if (childCtrl != nullptr && childCtrl->AllowCalc()) {
				childCtrl->CalculateLabelSize(&widthTextMax, &heightTextTotal);
				if (widthTextMax > maxX) maxX = widthTextMax;
			}

			if (parentSizer->GetOrientation() == wxOrientation::wxHORIZONTAL) {
				return;
			}

			for (unsigned int idx = 0; idx < child->GetChildCount(); idx++) {
				Calculate(child->GetChild(idx), sizer ? sizer : parentSizer, maxX);
			}
		};

		static inline void Apply(IValueFrame* child, wxBoxSizer* parentSizer, int& maxX) {

			if (child->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
				child = child->GetChild(0);
			}

			wxASSERT(child);

			wxObject* wx_object = child->GetWxObject();

			wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(wx_object);
			wxControlDynamicBorder* childCtrl = dynamic_cast<wxControlDynamicBorder*>(wx_object);

			int currMax = maxX;

			//if (maxX != wxNOT_FOUND ||
			//	parentSizer->GetOrientation() == wxOrientation::wxVERTICAL) {
			//	Calculate(child, sizer ? sizer : parentSizer, maxX);
			//}

			for (unsigned int idx = 0; idx < child->GetChildCount(); idx++) {
				Apply(child->GetChild(idx), sizer ? sizer : parentSizer, currMax);
			}

			if (childCtrl != nullptr && childCtrl->AllowCalc()) {

				if (maxX != wxNOT_FOUND) {
					childCtrl->ApplyLabelSize(wxSize(maxX, wxNOT_FOUND));
				}
				else {
					childCtrl->ApplyLabelSize(wxDefaultSize);
				}

				if (parentSizer->GetOrientation() == wxOrientation::wxHORIZONTAL) {
					maxX = wxNOT_FOUND;
				}
			}
		};

	public:

		static inline bool CalculateAndApply(const CValueForm* control) {

			int currMaxX = 0;

			wxBoxSizer* parentSizer = dynamic_cast<wxBoxSizer*>(control->GetWxObject());

			if (parentSizer->GetOrientation() == wxOrientation::wxVERTICAL) {
				Calculate(control, parentSizer, currMaxX);
			}

			for (unsigned int idx = 0; idx < control->GetChildCount(); idx++) {
				Apply(control->GetChild(idx), parentSizer, currMaxX);
			}

			return true;
		}
	};

	return CControlCalculateSize::CalculateAndApply(GetValueForm());
}

void IVisualHost::UpdateVirtualSize()
{
	int w, h, panelW, panelH;
	GetVirtualSize(&w, &h);
	GetBackgroundWindow()->GetSize(&panelW, &panelH);
	panelW += 50; panelH += 50;
	if (panelW != w || panelH != h)
		SetVirtualSize(panelW, panelH);

	//Refresh frame window
	wxScrolledWindow::Refresh();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

std::set<IVisualEditorNotebook*> IVisualEditorNotebook::ms_visualEditorArray = {};

/////////////////////////////////////////////////////////////////////////////////////////////////////

IVisualEditorNotebook::IVisualEditorNotebook()
{
	ms_visualEditorArray.insert(this);
}

IVisualEditorNotebook::~IVisualEditorNotebook()
{
	ms_visualEditorArray.erase(this);
}

IVisualEditorNotebook* IVisualEditorNotebook::FindEditorByForm(const IValueFrame* valueForm)
{
	for (auto& visualEditor : ms_visualEditorArray) {
		if (valueForm == visualEditor->GetValueForm())
			return visualEditor;
	}

	return nullptr;
}