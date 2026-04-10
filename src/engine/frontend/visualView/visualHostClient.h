#ifndef __VISUAL_EDITOR_VIEW_H__
#define __VISUAL_EDITOR_VIEW_H__

#include "visualHost.h"

#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/ctrl/sizer.h"
#include "frontend/visualView/ctrl/widgets.h"

class CVisualClientHost : public IVisualHost {
public:

	// ctor
	CVisualClientHost(CFormVisualDocument* document, CValueForm* valueForm, wxWindow* parent);
	virtual ~CVisualClientHost();

	virtual bool IsShownHost() const { return m_valueForm->IsShown(); }

	virtual CValueForm* GetValueForm() const { return m_valueForm; }
	virtual void SetValueForm(CValueForm* valueForm) { m_valueForm = valueForm; }

	virtual wxWindow* GetParentBackgroundWindow() const { return const_cast<CVisualClientHost*>(this); }
	virtual wxWindow* GetBackgroundWindow() const { return const_cast<CVisualClientHost*>(this); }

	virtual void OnClickFromApp(wxWindow* currentWindow, wxMouseEvent& event);

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

	bool m_dataViewSizeChanged;
	wxSize m_dataViewSize;
	CValuePtr<CValueForm> m_valueForm;
	CFormVisualDocument* m_document;
};

//********************************************************************************************
//*                                  Visual Document & View                                  *
//********************************************************************************************

class FRONTEND_API CFormVisualEditView : public CMetaView {
public:

	CFormVisualEditView() : m_visualHost(nullptr) {}
	virtual ~CFormVisualEditView();

	virtual wxPrintout* OnCreatePrintout() override;

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	virtual void OnClosingDocument() override;

	CVisualClientHost* GetVisualHost() const { return m_visualHost; }

private:
	CVisualClientHost* m_visualHost;
};

class FRONTEND_API CFormVisualDocument : public IMetaDataDocument {
public:

	CFormVisualDocument(CValueForm* valueForm);
	virtual ~CFormVisualDocument();

	virtual class IMetaData* GetMetaData() const;

	virtual bool IsVisualDemonstrationDoc() const { return false; }

	virtual bool OnCreate(const wxString& WXUNUSED(path), long flags) override;
	virtual bool OnCloseDocument() override;

	virtual bool IsCloseOnOwnerClose() const override;

	virtual bool IsModified() const override { return m_documentModified; }
	virtual void Modify(bool modify) override;
	virtual bool Save() override;
	virtual bool SaveAs() override { return true; }

	virtual void SetDocParent(CMetaDocument* docParent) override;

	CFormVisualEditView* GetFirstView() const;
	CValueForm* GetValueForm() const;
	const CUniqueKey& GetFormKey() const;
	bool CompareFormKey(const CUniqueKey& formKey) const;

	static CUniqueKey CreateFormUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	static CValueForm* FindFormByUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	static CValueForm* FindFormByUniqueKey(const CUniqueKey& guid);
	static CValueForm* FindFormByControlUniqueKey(const CUniqueKey& guid);
	static CValueForm* FindFormBySourceUniqueKey(const CUniqueKey& guid);

	static CFormVisualDocument* FindDocByUniqueKey(const CUniqueKey& guid);

	static bool UpdateFormUniqueKey(const CUniquePairKey& guid);

protected:
	virtual CMetaView* DoCreateView();
private:
	CValuePtr<CValueForm> m_valueForm;
};

class FRONTEND_API CFormVisualDemoDocument : public CFormVisualDocument {
public:

	CFormVisualDemoDocument(CValueForm* valueForm) :
		CFormVisualDocument(valueForm)
	{
	}

	virtual bool IsVisualDemonstrationDoc() const { return true; }
};

class FRONTEND_API CFormVisualCommandProcessor : public wxCommandProcessor {

public:
	virtual bool CanUndo() const { return false; }
	virtual bool CanRedo() const { return false; }
};

#endif