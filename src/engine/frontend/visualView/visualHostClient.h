#ifndef __VISUAL_EDITOR_VIEW_H__
#define __VISUAL_EDITOR_VIEW_H__

#include "visualHost.h"

#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/ctrl/sizer.h"
#include "frontend/visualView/ctrl/widgets.h"

class ibVisualHostClient : public ibVisualHost {
public:

	// ctor
	ibVisualHostClient(ibFormVisualDocument* document, ibValueForm* valueForm, wxWindow* parent);
	virtual ~ibVisualHostClient();

	virtual bool IsShownHost() const { return m_valueForm->IsShown(); }

	virtual ibValueForm* GetValueForm() const { return m_valueForm; }
	virtual void SetValueForm(ibValueForm* valueForm) { m_valueForm = valueForm; }

	virtual wxWindow* GetParentBackgroundWindow() const { return const_cast<ibVisualHostClient*>(this); }
	virtual wxWindow* GetBackgroundWindow() const { return const_cast<ibVisualHostClient*>(this); }

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
	ibValuePtr<ibValueForm> m_valueForm;
	ibFormVisualDocument* m_document;
};

//********************************************************************************************
//*                                  Visual Document & View                                  *
//********************************************************************************************

class FRONTEND_API ibFormVisualEditView : public ibMetaView {
public:

	ibFormVisualEditView() : m_visualHost(nullptr) {}
	virtual ~ibFormVisualEditView();

	virtual wxPrintout* OnCreatePrintout() override;

	virtual bool OnCreate(ibMetaDocument* doc, long flags) override;
	virtual void OnUpdate(wxView* sender, wxObject* hint = nullptr) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	virtual void OnClosingDocument() override;

	ibVisualHostClient* GetVisualHost() const { return m_visualHost; }

private:
	ibVisualHostClient* m_visualHost;
};

class FRONTEND_API ibFormVisualCommandProcessor : public wxCommandProcessor {

public:
	virtual bool CanUndo() const { return false; }
	virtual bool CanRedo() const { return false; }
};

class FRONTEND_API ibFormVisualDocument : public ibMetaDataDocument {
public:

	ibFormVisualDocument(ibValueForm* valueForm);
	virtual ~ibFormVisualDocument();

	virtual class ibMetaData* GetMetaData() const;

	virtual bool IsVisualDemonstrationDoc() const { return false; }

	virtual bool OnCreate(const wxString& WXUNUSED(path), long flags) override;
	virtual bool OnCloseDocument() override;

	virtual bool IsCloseOnOwnerClose() const override;

	virtual bool IsModified() const override { return m_documentModified; }
	virtual void Modify(bool modify) override;
	virtual bool Save() override;
	virtual bool SaveAs() override { return true; }

	virtual void SetDocParent(ibMetaDocument* docParent) override;

	ibFormVisualEditView* GetFirstView() const;
	ibValueForm* GetValueForm() const;
	const ibUniqueKey& GetFormKey() const;
	bool CompareFormKey(const ibUniqueKey& formKey) const;

	static ibUniqueKey CreateFormUniqueKey(const ibBackendControlFrame* ownerControl,
		const ibSourceDataObject* sourceObject, const ibUniqueKey& formGuid);

	static ibValueForm* FindFormByUniqueKey(const ibBackendControlFrame* ownerControl,
		const ibSourceDataObject* sourceObject, const ibUniqueKey& formGuid);

	static ibValueForm* FindFormByUniqueKey(const ibUniqueKey& guid);
	static ibValueForm* FindFormByControlUniqueKey(const ibUniqueKey& guid);
	static ibValueForm* FindFormBySourceUniqueKey(const ibUniqueKey& guid);

	static ibFormVisualDocument* FindDocByUniqueKey(const ibUniqueKey& guid);

	static bool UpdateFormUniqueKey(const ibUniqueKeyPair& guid);

protected:
	virtual ibMetaView* DoCreateView();
private:
	ibValuePtr<ibValueForm> m_valueForm;
};

class FRONTEND_API ibFormVisualDocumentDemo : public ibFormVisualDocument {
public:

	ibFormVisualDocumentDemo(ibValueForm* valueForm) :
		ibFormVisualDocument(valueForm)
	{
	}

	virtual bool IsVisualDemonstrationDoc() const { return true; }
};

#endif