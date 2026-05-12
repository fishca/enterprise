#include "webChildFrame.h"

#include <wx/docview.h>

#include "visualView/visualHostClient.h"

ibWebMDIChildFrame::ibWebMDIChildFrame(ibWebWindow* parent, const wxString& title)
{
	SetLabel(title);
	if (parent != nullptr)
		SetParent(parent);
}

ibWebDocChildFrame::ibWebDocChildFrame(wxDocument* doc, wxView* view,
	ibWebWindow* parent, const wxString& title)
	: ibWebMDIChildFrame(parent, title)
	, m_doc(doc)
	, m_view(view)
	, m_docManager(doc != nullptr ? doc->GetDocumentManager() : nullptr)
{
}

ibWebDocChildFrame::~ibWebDocChildFrame()
{
	// Tab holds non-owning pointers to view/doc. By the time tab
	// destruction reaches here, ibValueForm::CloseForm has already
	// run DeleteAllViews, which deleted each view AND — per
	// wxDocument's contract ("deleting the last view also deletes
	// the doc itself implicitly", see wxWidgets docview.cpp:239)
	// the document. Both m_view and m_doc are dangling at this
	// point, so we just drop the references.
	m_view = nullptr;
	m_doc  = nullptr;
}

ibVisualHostClient* ibWebDocChildFrame::GetHost() const
{
	// Route through the ibMetaView subclass that holds the per-tab
	// host. ibFormVisualEditView stores ibVisualHostClient; future
	// tab types (tabular doc view, text doc view, print preview)
	// will expose their own host classes through the same view
	// interface, so this single lookup covers them all.
	auto* view = dynamic_cast<ibFormVisualEditView*>(m_view);
	return view != nullptr ? view->GetVisualHost() : nullptr;
}
