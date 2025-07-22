#ifndef __VISUAL_EDITOR_VIEW_H__
#define __VISUAL_EDITOR_VIEW_H__

#include "visual.h"

#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/ctrl/sizers.h"
#include "frontend/visualView/ctrl/widgets.h"

class CVisualHost : public IVisualHost {
	CValueForm* m_valueForm;
	CMetaDocument* m_document;
	bool m_dataViewSizeChanged;
	wxSize m_dataViewSize;
public:

	// ctor
	CVisualHost(CMetaDocument* document, CValueForm* valueForm, wxWindow* parent);
	virtual ~CVisualHost();

	virtual bool IsShownHost() const { return m_valueForm->IsShown(); }

	virtual CValueForm* GetValueForm() const { return m_valueForm; }
	virtual void SetValueForm(CValueForm* valueForm) { m_valueForm = valueForm; }

	virtual wxWindow* GetParentBackgroundWindow() const { return const_cast<CVisualHost*>(this); }
	virtual wxWindow* GetBackgroundWindow() const { return const_cast<CVisualHost*>(this); }

	void ShowForm();
	void ActivateForm();
	void UpdateForm();
	bool CloseForm();

protected:

	virtual void SetCaption(const wxString& strCaption);
	virtual void SetOrientation(int orient);

protected:
	void OnSize(wxSizeEvent& event);
	void OnIdle(wxIdleEvent& event);
protected:
	friend class CValueForm;
};

class CVisualDemoHost : public CVisualHost {
public:

	CVisualDemoHost(CMetaDocument* document, CValueForm* valueForm, wxWindow* parent) :
		CVisualHost(document, valueForm, parent)
	{
	}

	virtual bool IsDemonstration() const { return true; }
};

#endif