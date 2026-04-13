////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual client host  
////////////////////////////////////////////////////////////////////////////

#include "visualHostClient.h"

CVisualClientHost::CVisualClientHost(CFormVisualDocument* document, CValueForm* valueForm, wxWindow* parent) :
	IVisualHost(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
	m_document(document),
	m_valueForm(valueForm),
	m_dataViewSize(wxDefaultSize),
	m_dataViewSizeChanged(false)
{
	CVisualClientHost::Bind(wxEVT_SIZE, &CVisualClientHost::OnSize, this);
	CVisualClientHost::Bind(wxEVT_IDLE, &CVisualClientHost::OnIdle, this);
}

CVisualClientHost::~CVisualClientHost()
{
	CVisualClientHost::Unbind(wxEVT_SIZE, &CVisualClientHost::OnSize, this);
	CVisualClientHost::Unbind(wxEVT_IDLE, &CVisualClientHost::OnIdle, this);

	ClearControl(m_valueForm, true);
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualClientHost::OnSize(wxSizeEvent& event)
{
	m_dataViewSizeChanged = (m_dataViewSize != GetSize()) && (m_dataViewSize != wxDefaultSize);
	event.Skip();
}

void CVisualClientHost::OnIdle(wxIdleEvent& event)
{
	if (m_dataViewSizeChanged)
		m_valueForm->RefreshForm();

	m_dataViewSize = GetSize();
	m_dataViewSizeChanged = false;

	event.Skip();
}

/////////////////////////////////////////////////////////////////////////////////

void CVisualClientHost::OnClickFromApp(wxWindow* currentWindow, wxMouseEvent& event)
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

void CVisualClientHost::SetCaption(const wxString& strCaption)
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

void CVisualClientHost::SetOrientation(int orient)
{
	const wxWindow* backgroundWindow = GetBackgroundWindow();
	wxASSERT(backgroundWindow);
	wxBoxSizer* createdBoxSizer = dynamic_cast<wxBoxSizer*>(backgroundWindow->GetSizer());
	if (createdBoxSizer != nullptr) createdBoxSizer->SetOrientation(orient);
	wxASSERT(createdBoxSizer);
}

/////////////////////////////////////////////////////////////////////////////////////////////
