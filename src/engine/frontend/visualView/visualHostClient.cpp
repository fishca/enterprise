////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual client host  
////////////////////////////////////////////////////////////////////////////

#include "visualHostClient.h"

ibVisualHostClient::ibVisualHostClient(ibFormVisualDocument* document, ibValueForm* valueForm, wxWindow* parent) :
	ibVisualHost(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
	m_document(document),
	m_valueForm(valueForm),
	m_dataViewSize(wxDefaultSize),
	m_dataViewSizeChanged(false)
{
	ibVisualHostClient::Bind(wxEVT_SIZE, &ibVisualHostClient::OnSize, this);
	ibVisualHostClient::Bind(wxEVT_IDLE, &ibVisualHostClient::OnIdle, this);
}

ibVisualHostClient::~ibVisualHostClient()
{
	ibVisualHostClient::Unbind(wxEVT_SIZE, &ibVisualHostClient::OnSize, this);
	ibVisualHostClient::Unbind(wxEVT_IDLE, &ibVisualHostClient::OnIdle, this);

	ClearControl(m_valueForm, true);
}

/////////////////////////////////////////////////////////////////////////////////

void ibVisualHostClient::OnSize(wxSizeEvent& event)
{
	m_dataViewSizeChanged = (m_dataViewSize != GetSize()) && (m_dataViewSize != wxDefaultSize);
	event.Skip();
}

void ibVisualHostClient::OnIdle(wxIdleEvent& event)
{
	if (m_dataViewSizeChanged)
		m_valueForm->RefreshForm();

	m_dataViewSize = GetSize();
	m_dataViewSizeChanged = false;

	event.Skip();
}

/////////////////////////////////////////////////////////////////////////////////

void ibVisualHostClient::OnClickFromApp(wxWindow* currentWindow, wxMouseEvent& event)
{
	if (event.GetEventType() == wxEVT_LEFT_DOWN) {
		wxWindow* wnd = currentWindow;
		while (wnd != nullptr) {
			ibValueFrame* founded = GetObjectBase(wnd);
			if (founded != nullptr) {
				OnSelected(founded, wnd);
				break;
			}
			wnd = wnd->GetParent();
		}
	}
}

#include "backend/metaCollection/partial/commonObject.h"

void ibVisualHostClient::SetCaption(const wxString& strCaption)
{
	const ibValueForm* handler = m_document->GetValueForm();

	if (m_document->IsVisualDemonstrationDoc()) {
		if (strCaption.IsEmpty()) {
			const ibSourceDataObject* srcObject = handler->GetSourceObject();
			if (srcObject != nullptr) {
				const ibValueMetaObjectFormBase* creator = handler->GetFormMetaObject();
				const ibValueMetaObjectGenericData* genericObject = srcObject->GetSourceMetaObject();
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
				const ibValueMetaObjectFormBase* creator = handler->GetFormMetaObject();
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
		const ibSourceDataObject* srcObject = handler->GetSourceObject();
		if (srcObject != nullptr && !m_document->IsVisualDemonstrationDoc()) {
			m_document->SetTitle(srcObject->GetSourceCaption());
			const ibValueMetaObjectGenericData* genericObject = srcObject->GetSourceMetaObject();
			m_document->SetFilename(genericObject->GetFileName(), true);
		}
		else {
			const ibValueMetaObjectFormBase* creator = handler->GetFormMetaObject();
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

void ibVisualHostClient::SetOrientation(int orient)
{
	const wxWindow* backgroundWindow = GetBackgroundWindow();
	wxASSERT(backgroundWindow);
	wxBoxSizer* createdBoxSizer = dynamic_cast<wxBoxSizer*>(backgroundWindow->GetSizer());
	if (createdBoxSizer != nullptr) createdBoxSizer->SetOrientation(orient);
	wxASSERT(createdBoxSizer);
}

/////////////////////////////////////////////////////////////////////////////////////////////
