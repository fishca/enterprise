#ifndef _INTERFACE_H__
#define _INTERFACE_H__

#include "frontend/docView/docView.h"

// ----------------------------------------------------------------------------
// Edit form classes
// ----------------------------------------------------------------------------

// The view using a standard wxTextCtrl to show its contents
class ibInterfaceEditView : public ibMetaView {
	class ibInterfaceEditor* m_interfaceEditor;
public:

	ibInterfaceEditView() : ibMetaView() {}

	virtual bool OnCreate(ibMetaDocument* doc, long flags) override;
	virtual void OnUpdate(wxView* sender, wxObject* hint) override;
	virtual void OnDraw(wxDC* dc) override;
	virtual bool OnClose(bool deleteWindow = true) override;

private:

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS(ibInterfaceEditView);
};

// ----------------------------------------------------------------------------
// ITextDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

class ibInterfacibDocument : public ibMetaDocument
{
public:
	ibInterfacibDocument() : ibMetaDocument() { m_childDoc = false; }

	virtual bool OnCreate(const wxString& path, long flags) override;

	virtual bool IsModified() const override;
	virtual void Modify(bool mod) override;

protected:

	virtual bool DoSaveDocument(const wxString& filename) override;
	virtual bool DoOpenDocument(const wxString& filename) override;

	wxDECLARE_NO_COPY_CLASS(ibInterfacibDocument);
	wxDECLARE_ABSTRACT_CLASS(ibInterfacibDocument);
};

// ----------------------------------------------------------------------------
// A very simple text document class
// ----------------------------------------------------------------------------

class ibInterfaceEditDocument : public ibInterfacibDocument
{
public:
	ibInterfaceEditDocument() : ibInterfacibDocument() { }

	wxDECLARE_NO_COPY_CLASS(ibInterfaceEditDocument);
	wxDECLARE_DYNAMIC_CLASS(ibInterfaceEditDocument);
};

#endif