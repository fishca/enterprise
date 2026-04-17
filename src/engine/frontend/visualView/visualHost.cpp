////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor
////////////////////////////////////////////////////////////////////////////

#include "visualHost.h"

#include "ctrl/control.h"
#include "ctrl/form.h"

#include "pageWindow.h"

#include <wx/collpane.h>

wxIMPLEMENT_ABSTRACT_CLASS(ibVisualHost, wxScrolledWindow)

#if !defined(__WXGTK__ )
#define __FREEZE_CONTROL__
#endif

namespace {

// RAII guard that freezes/thaws the parent background window for the
// duration of a layout-changing operation. Replaces the half-dozen
// copy-pasted #if __FREEZE_CONTROL__ blocks that used to wrap each
// Create/Update/Remove/Clear call and made early-return impossible
// without leaking a Thaw.
class ScopedFreeze {
public:
	explicit ScopedFreeze(wxWindow* window) : m_window(window) {
#if defined(__FREEZE_CONTROL__)
		if (m_window != nullptr) m_window->Freeze();
#endif
	}
	~ScopedFreeze() {
#if defined(__FREEZE_CONTROL__)
		if (m_window != nullptr) m_window->Thaw();
#endif
	}
	ScopedFreeze(const ScopedFreeze&) = delete;
	ScopedFreeze& operator=(const ScopedFreeze&) = delete;
private:
	wxWindow* m_window;
};

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibVisualHost::CreateVisualHost()
{
	const ibValueForm* valueForm = GetValueForm();

	ScopedFreeze freeze(GetParentBackgroundWindow());

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
			ibValueFrame* child = valueForm->GetChild(i);
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

#ifdef __WXOSX__
	GetParentBackgroundWindow()->Refresh();
	GetParentBackgroundWindow()->Update();
#endif

	return true;
}

bool ibVisualHost::UpdateVisualHost()
{
	const ibValueForm* valueForm = GetValueForm();

	ScopedFreeze freeze(GetParentBackgroundWindow());

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

			ibValueFrame* child = valueForm->GetChild(i);

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

#ifdef __WXOSX__
	GetParentBackgroundWindow()->Refresh();
	GetParentBackgroundWindow()->Update();
#endif

	return true;
}

bool ibVisualHost::ClearVisualHost()
{
	ScopedFreeze freeze(GetParentBackgroundWindow());

	ibValueForm* valueForm = GetValueForm();
	if (valueForm != nullptr) ClearControl(valueForm, true);

	GetParentBackgroundWindow()->DestroyChildren();
	GetParentBackgroundWindow()->SetSizer(nullptr); // *!*

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

ibValueFrame* ibVisualHost::GetObjectBase(const wxObject* wxobject) const
{
	if (nullptr == wxobject) {
		wxLogError(wxT("wxObject was nullptr!"));
		return nullptr;
	}

	for (auto& pair : m_baseObjects) {
		if (pair.second == wxobject)
			return pair.first;
	}

	wxLogError(wxT("No corresponding ibValueFrame for wxObject. Name: %s"), wxobject->GetClassInfo()->GetClassName());
	return nullptr;
}

wxObject* ibVisualHost::GetWxObject(const ibValueFrame* baseobject) const
{
	if (baseobject == nullptr) {
		wxLogError(wxT("baseobject was nullptr!"));
		return nullptr;
	}

	if (baseobject->GetComponentType() == COMPONENT_TYPE_FRAME)
		return GetFrameSizer();

	// The map is keyed by ibValueFrame* — use find() instead of the linear
	// scan this used to do. O(1) per call adds up on forms with hundreds
	// of controls since every CreateControl/UpdateControl walks the tree
	// and looks up each parent.
	const auto it = m_baseObjects.find(const_cast<ibValueFrame*>(baseobject));
	if (it != m_baseObjects.end())
		return it->second;

	wxLogError(wxT("No corresponding wxObject for ibValueFrame. Name: %s"), baseobject->GetClassName().c_str());
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void ibVisualHost::GenerateControl(ibValueFrame* obj, wxWindow* wxparent, wxObject* parent, bool firstCreated)
{
	wxObject* createdObject = Create(obj, wxparent);
	wxWindow* createdWindow = nullptr;

	switch (obj->GetComponentType())
	{
	case COMPONENT_TYPE_WINDOW:
		if (obj->GetClassName() == wxT("NotebookPage")) {
			if (ibPanelPage* pageWindow = wxDynamicCast(createdObject, ibPanelPage))
				createdWindow = pageWindow;
		}
		else {
			createdWindow = wxDynamicCast(createdObject, wxWindow);
		}
		break;

	case COMPONENT_TYPE_SIZER:
	case COMPONENT_TYPE_SIZERITEM:
		if (obj->GetClassName() == wxT("Staticboxsizer")) {
			if (wxStaticBoxSizer* s = wxDynamicCast(createdObject, wxStaticBoxSizer))
				createdWindow = s->GetStaticBox();
		}
		break;

	default: break;
	}

	AppendInnerControl(obj, createdObject);

	// Collapsible panes expose their inner pane as the real parent for
	// children; follow that redirection before recursing.
	if (wxCollapsiblePane* collpane = wxDynamicCast(createdObject, wxCollapsiblePane)) {
		createdWindow = collpane->GetPane();
		createdObject = createdWindow;
	}

	wxWindow* childParent = createdWindow != nullptr ? createdWindow : wxparent;
	for (unsigned int i = 0; i < obj->GetChildCount(); ++i)
		GenerateControl(obj->GetChild(i), childParent, createdObject, firstCreated);

	OnCreated(obj, createdObject, wxparent, firstCreated);
}

void ibVisualHost::RefreshControl(ibValueFrame* obj, wxWindow* wxparent, wxObject* parent)
{
	wxObject* createdObject = GetWxObject(obj);
	wxWindow* createdWindow = nullptr;
	wxSizer*  createdSizer  = nullptr;

	switch (obj->GetComponentType())
	{
	case COMPONENT_TYPE_WINDOW:
		createdWindow = wxDynamicCast(createdObject, wxWindow);
		break;

	case COMPONENT_TYPE_SIZER:
	case COMPONENT_TYPE_SIZERITEM:
		if (obj->GetClassName() == wxT("Staticboxsizer")) {
			if (wxStaticBoxSizer* s = wxDynamicCast(createdObject, wxStaticBoxSizer)) {
				createdWindow = s->GetStaticBox();
				createdSizer  = s;
			}
		}
		else {
			createdSizer = wxDynamicCast(createdObject, wxSizer);
		}
		break;

	default: break;
	}

	Update(obj, createdObject);

	if (wxCollapsiblePane* collpane = wxDynamicCast(createdObject, wxCollapsiblePane)) {
		createdWindow = collpane->GetPane();
		createdObject = createdWindow;
	}

	wxWindow* childParent = createdWindow != nullptr ? createdWindow : wxparent;
	for (unsigned int i = 0; i < obj->GetChildCount(); ++i)
		RefreshControl(obj->GetChild(i), childParent, createdObject);

	OnUpdated(obj, createdObject, wxparent);

	// If we just produced a sizer sitting directly under a window, attach
	// it as that window's layout sizer and run the initial layout pass.
	const bool parentIsWindow = wxDynamicCast(parent, wxWindow) != nullptr;
	if (createdSizer != nullptr && (parentIsWindow || parent == nullptr)) {
		wxparent->SetSizer(createdSizer);
		if (parent != nullptr)
			createdSizer->SetSizeHints(wxparent);
		wxparent->SetAutoLayout(true);
		wxparent->Layout();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

// If objParent is a wxStaticBoxSizer, trigger a layout pass on it so the
// newly inserted / updated / removed child is repositioned within the
// box. Shared by Create/Update/RemoveControl.
inline void RelayoutStaticBoxIfAny(ibValueFrame* objParent, wxObject* parentObj)
{
	if (objParent != nullptr && objParent->GetClassName() == wxT("Staticboxsizer")) {
		if (wxStaticBoxSizer* s = dynamic_cast<wxStaticBoxSizer*>(parentObj))
			s->Layout();
	}
}

} // namespace

ibVisualHost::ControlContext ibVisualHost::ResolveControlContext(
	ibValueFrame* obj, ibValueFrame* parent, bool resolveWindow) const
{
	ControlContext ctx{};
	ctx.objControl = obj;
	ctx.objParent  = parent != nullptr ? parent : obj->GetParent();

	// Unwrap a SIZERITEM wrapper so we operate on the actual parent sizer.
	if (ctx.objParent != nullptr &&
		ctx.objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
	{
		ctx.objControl = ctx.objParent;
		ctx.parentObj  = GetWxObject(ctx.objParent->GetParent());
		ctx.objParent  = ctx.objParent->GetParent();
	}
	else {
		ctx.parentObj = GetWxObject(ctx.objParent);
	}

	if (!resolveWindow)
		return ctx;

	// A static-box sizer embeds its own static-box window as the child's
	// wxWindow parent. Otherwise fall back to a window cast of parentObj
	// and walk up the tree looking for the nearest COMPONENT_TYPE_WINDOW
	// ancestor; finally use the host's background window.
	if (ctx.objParent != nullptr &&
		ctx.objParent->GetClassName() == wxT("Staticboxsizer"))
	{
		wxStaticBoxSizer* s = dynamic_cast<wxStaticBoxSizer*>(ctx.parentObj);
		wxASSERT(s);
		ctx.windowObj = s->GetStaticBox();
	}
	else {
		ctx.windowObj = dynamic_cast<wxWindow*>(ctx.parentObj);
	}

	for (ibValueFrame* up = ctx.objParent; !ctx.windowObj && up != nullptr; up = up->GetParent()) {
		if (up->GetComponentType() == COMPONENT_TYPE_WINDOW) {
			ctx.windowObj = dynamic_cast<wxWindow*>(GetWxObject(up));
			break;
		}
	}

	if (ctx.windowObj == nullptr)
		ctx.windowObj = GetBackgroundWindow();

	return ctx;
}

void ibVisualHost::CreateControl(ibValueFrame* obj, ibValueFrame* parent, bool firstCreated)
{
	const ControlContext ctx = ResolveControlContext(obj, parent);

	ScopedFreeze freeze(GetParentBackgroundWindow());

	GenerateControl(ctx.objControl, ctx.windowObj, ctx.parentObj, firstCreated);
	RefreshControl(ctx.objControl, ctx.windowObj, ctx.parentObj);
	RelayoutStaticBoxIfAny(ctx.objParent, ctx.parentObj);

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();
}

void ibVisualHost::UpdateControl(ibValueFrame* obj, ibValueFrame* parent)
{
	const ControlContext ctx = ResolveControlContext(obj, parent);

	ScopedFreeze freeze(GetParentBackgroundWindow());

	RefreshControl(ctx.objControl, ctx.windowObj, ctx.parentObj);
	RelayoutStaticBoxIfAny(ctx.objParent, ctx.parentObj);

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();
}

void ibVisualHost::RemoveControl(ibValueFrame* obj, ibValueFrame* parent)
{
	const ControlContext ctx = ResolveControlContext(obj, parent, /*resolveWindow*/false);

	ScopedFreeze freeze(GetParentBackgroundWindow());

	ClearControl(ctx.objControl);
	RelayoutStaticBoxIfAny(ctx.objParent, ctx.parentObj);

	CalculateLabelSize(obj);
	UpdateHostSize();
	UpdateVirtualSize();
}

#include "frontend/win/ctrls/dynamicBorder.h"

void ibVisualHost::ClearControl(ibValueFrame* control, bool force)
{
	// Post-order: tear children down before the parent so destroy callbacks
	// see a consistent hierarchy.
	for (unsigned int i = 0; i < control->GetChildCount(); ++i)
		ClearControl(control->GetChild(i), force);

	switch (control->GetComponentType())
	{
	case COMPONENT_TYPE_WINDOW:
	{
		wxWindow* controlWnd = dynamic_cast<wxWindow*>(GetWxObject(control));
		if (controlWnd == nullptr)
			return;

		wxWindow* controlParent = controlWnd->GetParent();
		Cleanup(control, controlWnd);
		RemoveInnerControl(control);
		controlWnd->DeletePendingEvents();
		controlWnd->Destroy();
		if (!force && controlParent != nullptr)
			controlParent->Layout();
		break;
	}

	case COMPONENT_TYPE_SIZER:
	{
		wxSizer* controlSizer = dynamic_cast<wxSizer*>(GetWxObject(control));
		if (controlSizer == nullptr)
			return;

		ibValueFrame* controlParent = control->GetParent();
		Cleanup(control, controlSizer);
		RemoveInnerControl(control);

		// Unwrap SIZERITEM wrapper to get to the logical parent container.
		if (controlParent != nullptr &&
			controlParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
		{
			controlParent = controlParent->GetParent();
		}

		wxSizer* controlParentSizer = nullptr;
		if (controlParent != nullptr && controlParent->GetClassName() == wxT("NotebookPage")) {
			ibPanelPage* pageWnd = dynamic_cast<ibPanelPage*>(GetWxObject(controlParent));
			wxASSERT(pageWnd);
			controlParentSizer = pageWnd->GetSizer();
		}
		else {
			controlParentSizer = dynamic_cast<wxSizer*>(GetWxObject(controlParent));
		}

		if (controlParentSizer != nullptr)
			controlParentSizer->Detach(controlSizer);
		wxDELETE(controlSizer);
		if (!force && controlParentSizer != nullptr)
			controlParentSizer->Layout();
		break;
	}

	case COMPONENT_TYPE_FRAME:
		// Form root — nothing to delete here; children were torn down above.
		break;

	default:
	{
		wxObject* controlObj = GetWxObject(control);
		if (controlObj != nullptr) {
			Cleanup(control, controlObj);
			RemoveInnerControl(control);
			wxDELETE(controlObj);
		}
		break;
	}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibVisualHost::CalculateLabelSize(ibValueFrame* control)
{
	// Two-pass alignment of label columns across a form's layout tree.
	//
	//   Pass 1 (Calculate): walk the current vertical sizer, find every
	//   child that exposes ibControlDynamicBorder, and record the widest
	//   natural label width among them. Horizontal sizers are treated as
	//   a single unit — their contents do not share a vertical column.
	//
	//   Pass 2 (Apply): feed that width back to each contributing control
	//   via ApplyLabelSize(). Labels in the same vertical run line up on
	//   one grid, so their trailing controls (text fields, combos, ...)
	//   start at the same x offset.
	//
	// Apply uses post-order traversal so nested vertical groups compute
	// their own local max before the outer group aligns top-level items.
	// A horizontal sizer resets the outer max after its label is handled,
	// giving each vertical run its own fresh column budget.
	struct ibLabelAligner {

		// Recursively collect the max label width within a vertical sizer.
		static void Calculate(const ibValueFrame* node,
			wxBoxSizer* parentSizer, int& maxLabelWidth)
		{
			if (node->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
				node = node->GetChild(0);
			wxASSERT(node);

			wxObject* wxObj = node->GetWxObject();
			ibControlDynamicBorder* label =
				dynamic_cast<ibControlDynamicBorder*>(wxObj);

			if (label != nullptr && label->AllowCalc()) {
				wxCoord w = 0, h = 0;
				label->CalculateLabelSize(&w, &h);
				if (w > maxLabelWidth)
					maxLabelWidth = w;
			}

			// A horizontal sizer stops the column — its children sit on
			// one row, not stacked vertically.
			if (parentSizer->GetOrientation() == wxHORIZONTAL)
				return;

			wxBoxSizer* childSizer = dynamic_cast<wxBoxSizer*>(wxObj);
			wxBoxSizer* nextParent = childSizer != nullptr ? childSizer : parentSizer;
			for (unsigned int i = 0; i < node->GetChildCount(); ++i)
				Calculate(node->GetChild(i), nextParent, maxLabelWidth);
		}

		// Recursively apply the computed width back to each label.
		static void Apply(ibValueFrame* node,
			wxBoxSizer* parentSizer, int& maxLabelWidth)
		{
			if (node->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
				node = node->GetChild(0);
			wxASSERT(node);

			wxObject* wxObj = node->GetWxObject();
			wxBoxSizer* childSizer = dynamic_cast<wxBoxSizer*>(wxObj);
			ibControlDynamicBorder* label =
				dynamic_cast<ibControlDynamicBorder*>(wxObj);

			// Children see a snapshot of our current max; any adjustments
			// they make stay local to their subtree.
			int subtreeMax = maxLabelWidth;
			wxBoxSizer* nextParent = childSizer != nullptr ? childSizer : parentSizer;
			for (unsigned int i = 0; i < node->GetChildCount(); ++i)
				Apply(node->GetChild(i), nextParent, subtreeMax);

			if (label != nullptr && label->AllowCalc()) {
				label->ApplyLabelSize(maxLabelWidth != wxNOT_FOUND
					? wxSize(maxLabelWidth, wxNOT_FOUND)
					: wxDefaultSize);

				// After a label in a horizontal sizer we break the column
				// so the next vertical run starts from zero.
				if (parentSizer->GetOrientation() == wxHORIZONTAL)
					maxLabelWidth = wxNOT_FOUND;
			}
		}

		static bool CalculateAndApply(const ibValueForm* form)
		{
			wxBoxSizer* rootSizer =
				dynamic_cast<wxBoxSizer*>(form->GetWxObject());
			if (rootSizer == nullptr)
				return false;

			int maxLabelWidth = 0;
			if (rootSizer->GetOrientation() == wxVERTICAL)
				Calculate(form, rootSizer, maxLabelWidth);

			for (unsigned int i = 0; i < form->GetChildCount(); ++i)
				Apply(form->GetChild(i), rootSizer, maxLabelWidth);

			return true;
		}
	};

	return ibLabelAligner::CalculateAndApply(GetValueForm());
}

void ibVisualHost::UpdateVirtualSize()
{
	int w, h, panelW, panelH;
	GetVirtualSize(&w, &h);
	GetBackgroundWindow()->GetSize(&panelW, &panelH);
	panelW += 50; panelH += 50;
	if (panelW != w || panelH != h)
		SetVirtualSize(panelW, panelH);

	//Refresh frame window
	wxScrolledCanvas::Refresh();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

std::set<ibFrontendVisualEditorNotebook*> ibFrontendVisualEditorNotebook::ms_visualEditorArray = {};

/////////////////////////////////////////////////////////////////////////////////////////////////////

ibFrontendVisualEditorNotebook::ibFrontendVisualEditorNotebook()
{
	ms_visualEditorArray.insert(this);
}

ibFrontendVisualEditorNotebook::~ibFrontendVisualEditorNotebook()
{
	ms_visualEditorArray.erase(this);
}

ibFrontendVisualEditorNotebook* ibFrontendVisualEditorNotebook::FindEditorByForm(const ibValueFrame* valueForm)
{
	for (auto& visualEditor : ms_visualEditorArray) {
		if (valueForm == visualEditor->GetValueForm())
			return visualEditor;
	}

	return nullptr;
}