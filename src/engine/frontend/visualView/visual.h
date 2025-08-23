#ifndef _VISUAL_EDITOR_BASE_H__
#define _VISUAL_EDITOR_BASE_H__

#include <set>

#include <wx/artprov.h>
#include <wx/config.h>
#include <wx/cmdproc.h>
#include <wx/docview.h>
#include <wx/splitter.h>
#include <wx/spinbutt.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>

#include <map>

#include "frontend/frontend.h"

class FRONTEND_API IValueFrame;

class FRONTEND_API IVisualHost : public wxScrolledWindow {
	wxDECLARE_ABSTRACT_CLASS(IVisualHost);
public:

	IVisualHost(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxScrolledWindowStyle) : wxScrolledWindow(parent, id, pos, size, style | wxBORDER_SUNKEN)
	{
		wxScrolledWindow::SetDoubleBuffered(true);
		wxScrolledWindow::SetScrollRate(5, 5);
	}

	virtual ~IVisualHost() {/* ClearVisualHost(); */ }

	IValueFrame* GetObjectBase(const wxObject* wxobject) const;
	wxObject* GetWxObject(const IValueFrame* baseobject) const;
	wxSizer* GetFrameSizer() const { return GetBackgroundWindow()->GetSizer(); }

	bool CreateAndUpdateVisualHost() {
		return ClearVisualHost() &&
			CreateVisualHost() && UpdateVisualHost();
	}

	bool CreateVisualHost();
	bool UpdateVisualHost();
	bool ClearVisualHost();

	virtual bool IsShownHost() const { return true; }
	virtual bool IsDesignerHost() const { return false; }

	virtual class CValueForm* GetValueForm() const = 0;

	virtual wxWindow* GetParentBackgroundWindow() const = 0;
	virtual wxWindow* GetBackgroundWindow() const = 0;

	virtual void OnClickFromApp(wxWindow* currentWindow, wxMouseEvent& event) {}

protected:

	virtual void SetCaption(const wxString& strCaption) = 0;
	virtual void SetOrientation(int orient) = 0;

	virtual void UpdateHostSize() {}

private:

	//Generate component 
	void GenerateControl(IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject, bool firstCreated = false);

	//Update component
	void RefreshControl(IValueFrame* obj, wxWindow* wxparent, wxObject* parentObject);

protected:

	friend class CValueTableBox;

	friend class CValueNotebook;
	friend class CValueNotebookPage;

	friend class CVisualDocument;

	//Insert new control
	void CreateControl(IValueFrame* obj, IValueFrame* parent = nullptr, bool firstCreated = false);

	//Update exist control
	void UpdateControl(IValueFrame* obj, IValueFrame* parent = nullptr);

	//Remove control
	void RemoveControl(IValueFrame* obj, IValueFrame* parent = nullptr);

	// Give components an opportunity to cleanup
	void ClearControl(IValueFrame* control, bool force = false);

	// Calculate label size for static text
	bool CalculateLabelSize(IValueFrame* control = nullptr);

	//Update virtual size
	void UpdateVirtualSize();

	//*********************************************************
	//*                 Events for visual                     *
	//*********************************************************

	/**
	* Create an instance of the wxObject and return a pointer
	*/
	virtual wxObject* Create(IValueFrame* control, wxWindow* wndParent);

	/**
	* Allows components to do something after they have been created.
	* For example, Abstract components like NotebookPage and SizerItem can
	* add the actual widget to the Notebook or sizer.
	*
	* @param wxobject The object which was just created.
	* @param wxparent The wxWidgets parent - the wxObject that the created object was added to.
	*/
	virtual void OnCreated(IValueFrame* control, wxObject* obj, wxWindow* wndParent, bool first—reated = false);

	/**
	* Allows components to respond when selected in object tree.
	* For example, when a wxNotebook's page is selected, it can switch to that page
	*/
	virtual void OnSelected(IValueFrame* control, wxObject* obj);

	/**
	* Allows components to do something after they have been updated.
	*/
	virtual void Update(IValueFrame* control, wxObject* obj);

	/**
	* Allows components to do something after they have been updated.
	* For example, Abstract components like NotebookPage and SizerItem can
	* add the actual widget to the Notebook or sizer.
	*
	* @param wxobject The object which was just updated.
	* @param wxparent The wxWidgets parent - the wxObject that the updated object was added to.
	*/
	virtual void OnUpdated(IValueFrame* control, wxObject* obj, wxWindow* wndParent);

	/**
	 * Cleanup (do the reverse of Create)
	 */
	virtual void Cleanup(IValueFrame* control, wxObject* obj);

private:

	inline void AppendInnerControl(IValueFrame* control, wxObject* wx_object) {
		m_baseObjects.insert_or_assign(control, wx_object);
	}

	inline void RemoveInnerControl(IValueFrame* control) {
		m_baseObjects.erase(control);
	}

protected:

	//controls
	std::unordered_map<IValueFrame*, wxObject* > m_baseObjects;
};

class FRONTEND_API IVisualEditorNotebook {
	static std::set<IVisualEditorNotebook*> ms_visualEditorArray;
public:

	static IVisualEditorNotebook* FindEditorByForm(const IValueFrame* valueForm);

	IVisualEditorNotebook();
	virtual ~IVisualEditorNotebook();

	virtual IVisualHost* GetVisualHost() const = 0;

	virtual void CreateControl(const wxString& controlName) = 0;
	virtual void RemoveControl(IValueFrame* obj) = 0;
	virtual void CutControl(IValueFrame* obj, bool force = false) = 0;
	virtual void CopyControl(IValueFrame* obj) = 0;
	virtual bool PasteControl(IValueFrame* parent) = 0;
	virtual void InsertControl(IValueFrame* obj, IValueFrame* parent) = 0;
	virtual void ExpandControl(IValueFrame* obj, bool expand) = 0;
	virtual void SelectControl(IValueFrame* obj) = 0;

	virtual void ModifyEvent(class IEvent* event, const wxVariant& oldValue, const wxVariant& newValue) = 0;
	virtual void ModifyProperty(class IProperty* prop, const wxVariant& oldValue, const wxVariant& newValue) = 0;

	virtual IValueFrame* GetValueForm() const = 0;
	virtual void RefreshEditor() = 0;

	virtual wxEvtHandler* GetHighlightPaintHandler(wxWindow* wnd) const = 0;
};

#define g_visualHostContext FindVisualEditor()

#endif 