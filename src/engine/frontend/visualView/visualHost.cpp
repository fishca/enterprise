////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual host  
////////////////////////////////////////////////////////////////////////////

#include "visualHost.h"
#include <wx/collpane.h>

CVisualHost::CVisualHost(CVisualDocument* document, CValueForm* valueForm, wxWindow* parent) : IVisualHost(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
m_document(document), m_valueForm(valueForm), m_dataViewSize(wxDefaultSize), m_dataViewSizeChanged(false) {

	CVisualHost::Bind(wxEVT_SIZE, &CVisualHost::OnSize, this);
	CVisualHost::Bind(wxEVT_IDLE, &CVisualHost::OnIdle, this);
}

CVisualHost::~CVisualHost()
{
	CVisualHost::Unbind(wxEVT_SIZE, &CVisualHost::OnSize, this);
	CVisualHost::Unbind(wxEVT_IDLE, &CVisualHost::OnIdle, this);

	ClearControl(m_valueForm, true);
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualHost::OnSize(wxSizeEvent& event)
{
	m_dataViewSizeChanged = (m_dataViewSize != GetSize()) && (m_dataViewSize != wxDefaultSize);
	event.Skip();
}

void CVisualHost::OnIdle(wxIdleEvent& event)
{
	if (m_dataViewSizeChanged)
		m_valueForm->RefreshForm();

	m_dataViewSize = GetSize();
	m_dataViewSizeChanged = false;

	event.Skip();
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualHost::OnClickFromApp(wxWindow* currentWindow, wxMouseEvent& event)
{
	if (event.GetEventType() == wxEVT_LEFT_DOWN) {
		wxWindow* wnd = currentWindow;
		while (wnd != nullptr) {
			IValueFrame* founded = GetObjectBase(wnd);
			if (founded != nullptr) {
				OnSelected(founded, wnd);
				break;
			}
			wnd = wnd->GetParent();
		}
	}
}

#include "backend/metaCollection/partial/commonObject.h"

void CVisualHost::SetCaption(const wxString& strCaption)
{
	const CValueForm* handler = m_document->GetValueForm();

	if (m_document->IsVisualDemonstrationDoc()) {
		if (strCaption.IsEmpty()) {
			const ISourceDataObject* srcObject = handler->GetSourceObject();
			if (srcObject != nullptr) {
				const IValueMetaObjectForm* creator = handler->GetFormMetaObject();
				const IValueMetaObjectGenericData* genericObject = srcObject->GetSourceMetaObject();
				if (genericObject != nullptr) {
					m_document->SetTitle(genericObject->GetSynonym() + wxT(": ") + creator->GetSynonym());
					m_document->SetFilename(genericObject->GetFileName(), true);
				}
				else if (creator != nullptr) {
					m_document->SetTitle(creator->GetSynonym());
					m_document->SetFilename(creator->GetFileName(), true);
				}
			}
			else {
				const IValueMetaObjectForm* creator = handler->GetFormMetaObject();
				if (creator != nullptr) {
					m_document->SetTitle(creator->GetSynonym());
					m_document->SetFilename(creator->GetFileName(), true);
				}
			}
		}
		else {
			m_document->SetTitle(strCaption);
			m_document->SetFilename(wxEmptyString, true);
		}
	}
	else if (strCaption.IsEmpty()) {
		const ISourceDataObject* srcObject = handler->GetSourceObject();
		if (srcObject != nullptr && !m_document->IsVisualDemonstrationDoc()) {
			m_document->SetTitle(srcObject->GetSourceCaption());
			const IValueMetaObjectGenericData* genericObject = srcObject->GetSourceMetaObject();
			m_document->SetFilename(genericObject->GetFileName(), true);
		}
		else {
			const IValueMetaObjectForm* creator = handler->GetFormMetaObject();
			if (creator != nullptr && !m_document->IsVisualDemonstrationDoc()) {
				m_document->SetTitle(creator->GetSynonym());
				m_document->SetFilename(creator->GetFileName(), true);
			}
		}
	}
	else if (m_document != nullptr && !m_document->IsVisualDemonstrationDoc()) {
		m_document->SetTitle(strCaption);
		m_document->SetFilename(wxEmptyString, true);
	}
}

void CVisualHost::SetOrientation(int orient)
{
	const wxWindow* backgroundWindow = GetBackgroundWindow();
	wxASSERT(backgroundWindow);
	wxBoxSizer* createdBoxSizer = dynamic_cast<wxBoxSizer*>(backgroundWindow->GetSizer());
	if (createdBoxSizer != nullptr) createdBoxSizer->SetOrientation(orient);
	wxASSERT(createdBoxSizer);
}
