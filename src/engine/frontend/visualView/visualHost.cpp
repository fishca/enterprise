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

	m_valueForm->IncrRef();
}

CVisualHost::~CVisualHost()
{
	CVisualHost::Unbind(wxEVT_SIZE, &CVisualHost::OnSize, this);
	CVisualHost::Unbind(wxEVT_IDLE, &CVisualHost::OnIdle, this);

	ClearControl(m_valueForm, true);
	m_valueForm->DecrRef();
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

#include "backend/metaCollection/partial/commonObject.h"

void CVisualHost::SetCaption(const wxString& strCaption)
{
	const CValueForm* handler = m_document->GetValueForm();

	if (m_document->IsVisualDemonstrationDoc()) {
		if (strCaption.IsEmpty()) {
			const ISourceDataObject* srcObject = handler->GetSourceObject();
			if (srcObject != nullptr) {
				const IMetaObjectForm* metaFormObject = handler->GetFormMetaObject();
				const IMetaObjectGenericData* genericObject = srcObject->GetSourceMetaObject();
				if (genericObject != nullptr) {
					m_document->SetTitle(genericObject->GetSynonym() + wxT(": ") + metaFormObject->GetSynonym());
					m_document->SetFilename(genericObject->GetSynonym() + wxT(": ") + metaFormObject->GetSynonym(), true);
				}
				else if (metaFormObject != nullptr) {
					m_document->SetTitle(metaFormObject->GetSynonym());
					m_document->SetFilename(metaFormObject->GetSynonym(), true);
				}
			}
			else {
				const IMetaObjectForm* metaFormObject = handler->GetFormMetaObject();
				if (metaFormObject != nullptr) {
					m_document->SetTitle(metaFormObject->GetSynonym());
					m_document->SetFilename(metaFormObject->GetSynonym(), true);
				}
			}
		}
		else {
			m_document->SetTitle(strCaption);
			m_document->SetFilename(strCaption, true);
		}
	}
	else if (strCaption.IsEmpty()) {
		const ISourceDataObject* srcObject = handler->GetSourceObject();
		if (srcObject != nullptr && !m_document->IsVisualDemonstrationDoc()) {
			m_document->SetTitle(srcObject->GetSourceCaption());
			m_document->SetFilename(srcObject->GetSourceCaption(), true);
		}
		else {
			const IMetaObjectForm* metaObject = handler->GetFormMetaObject();
			if (metaObject != nullptr && !m_document->IsVisualDemonstrationDoc()) {
				m_document->SetTitle(metaObject->GetSynonym());
				m_document->SetFilename(metaObject->GetSynonym(), true);
			}
		}
	}
	else if (m_document != nullptr && !m_document->IsVisualDemonstrationDoc()) {
		m_document->SetTitle(strCaption);
		m_document->SetFilename(strCaption, true);
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
