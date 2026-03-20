#include "visualHostClient.h"

#include "backend/metaCollection/partial/commonObject.h"

static std::set<CFormVisualDocument*> s_createdDocFormArray = {};

//********************************************************************************************
//*                                  Visual Document & View                                  *
//********************************************************************************************

CFormVisualEditView* CFormVisualDocument::GetFirstView() const
{
	return wxDynamicCast(
		CMetaDocument::GetFirstView(), CFormVisualEditView
	);
}

const CUniqueKey& CFormVisualDocument::GetFormKey() const
{
	return m_valueForm->GetFormKey();
}

bool CFormVisualDocument::CompareFormKey(const CUniqueKey& formKey) const
{
	return m_valueForm->CompareFormKey(formKey);
}

CValueForm* CFormVisualDocument::GetValueForm() const
{
	return m_valueForm;
}

bool CFormVisualDocument::OnCreate(const wxString& path, long flags)
{
	const ISourceDataObject* sourceObject = m_valueForm->GetSourceObject();

	if (sourceObject != nullptr && !IsVisualDemonstrationDoc()) {
		const IValueMetaObjectGenericData* genericObject = sourceObject->GetSourceMetaObject();
		if (genericObject != nullptr) {
			CFormVisualDocument::SetIcon(genericObject->GetIcon());
			CFormVisualDocument::SetFilename(genericObject->GetFileName());
		}
	}
	else {
		const IValueMetaObjectForm* creator = m_valueForm->GetFormMetaObject();
		if (creator != nullptr) {
			CFormVisualDocument::SetIcon(creator->GetIcon());
		}
		CFormVisualDocument::SetFilename(creator->GetFileName());
	}

	CFormVisualDocument::SetTitle(m_valueForm->GetCaption());

	if (IsVisualDemonstrationDoc()) m_childDoc = false;

	return CMetaDocument::OnCreate(path, flags);
}

bool CFormVisualDocument::OnCloseDocument()
{
	if (m_valueForm != nullptr)
		m_valueForm->m_formModified = false;

	wxDocManager* documentManager = GetDocumentManager();

	// When the parent document closes, its children must be closed as well as
	// they can't exist without the parent.
	CMetaDocument const* documentParent = m_documentParent;

	if (documentManager != nullptr && documentParent != nullptr)
		documentManager->ActivateView(documentParent->GetFirstView());

	return CMetaDocument::OnCloseDocument();
}

bool CFormVisualDocument::IsCloseOnOwnerClose() const
{
	if (m_valueForm != nullptr)
		return m_valueForm->IsCloseOnOwnerClose();
	return true;
}

void CFormVisualDocument::Modify(bool modify)
{
	if (m_valueForm != nullptr)
		m_valueForm->m_formModified = modify;

	if (modify != m_documentModified) {

		m_documentModified = modify;

		// Allow views to append asterix to the title
		CFormVisualEditView* view = GetFirstView();
		if (view != nullptr) view->OnChangeFilename();
	}
}

#include "backend/system/systemManager.h"

bool CFormVisualDocument::Save()
{
	ISourceDataObject* sourceObject = m_valueForm != nullptr ?
		m_valueForm->GetSourceObject() : nullptr;

	bool success = true;

	try {
		success = sourceObject != nullptr ?
			sourceObject->SaveModify() : true;
	}
	catch (const CBackendAccessException* err) {
		CSystemFunction::Alert(err->GetErrorDescription());
		success = false;
	}
	catch (const CBackendException*) {
		CSystemFunction::Alert(_("An error occurred while trying to save the form!"));
		success = false;
	}

	if (success) {
		CFormVisualDocument::Modify(false);
		return true;
	}

	return true;
}

void CFormVisualDocument::SetDocParent(CMetaDocument* docParent)
{
	CMetaDocument::SetDocParent(docParent);

	if (docParent == nullptr &&
		(m_valueForm != nullptr && m_valueForm->m_controlOwner != nullptr)) {

		m_valueForm->m_controlOwner->ControlDecrRef();
		m_valueForm->m_controlOwner = nullptr;
	}
}

CMetaView* CFormVisualDocument::DoCreateView()
{
	return new CFormVisualEditView();
}

/////////////////////////////////////////////////////////////////////////////////////////////

CFormVisualDocument::CFormVisualDocument(CValueForm* valueForm)
	: m_valueForm(valueForm) {

	if (m_valueForm != nullptr) {

		CFormVisualDocument::SetCommandProcessor(new CFormVisualCommandProcessor);
		CFormVisualDocument::SetMetaObject(nullptr);
	}

	s_createdDocFormArray.insert(this);
}

CFormVisualDocument::~CFormVisualDocument()
{
	s_createdDocFormArray.erase(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

IMetaData* CFormVisualDocument::GetMetaData() const
{
	return m_valueForm != nullptr ?
		m_valueForm->GetMetaData() : nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

CUniqueKey CFormVisualDocument::CreateFormUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formKey)
{
	if (formKey.isValid()) {
		// 1. if set guid from user
		return formKey;
	}
	else if (ownerControl != nullptr) {
		// 2. if set guid in owner
		return ownerControl->GetControlGuid();
	}
	else if (sourceObject != nullptr) {
		//3. if set guid in object 
		return sourceObject->GetGuid();
	}

	//4. just generate 
	return wxNewUniqueGuid;
}

CValueForm* CFormVisualDocument::FindFormByUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formKey)
{
	return FindFormByUniqueKey(
		CreateFormUniqueKey(ownerControl, sourceObject, formKey)
	);
}

CValueForm* CFormVisualDocument::FindFormByUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {

		std::set<CFormVisualDocument*>::iterator foundedForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CFormVisualDocument* visualDoc) {
					return visualDoc->CompareFormKey(formKey);
				}
			);

		if (foundedForm != s_createdDocFormArray.end()) {
			CFormVisualDocument* foundedVisualDocument = *foundedForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

CValueForm* CFormVisualDocument::FindFormByControlUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {
		std::set<CFormVisualDocument*>::iterator foundedSourceForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CFormVisualDocument* visualDoc) {
					wxASSERT(visualDoc);
					CValueForm* valueForm = visualDoc->GetValueForm();
					wxASSERT(valueForm);
					IValueFrame* ownerControl = valueForm->GetOwnerControl();
					if (ownerControl != nullptr) return formKey == ownerControl->GetControlGuid();
					return false;
				}
			);

		if (foundedSourceForm != s_createdDocFormArray.end()) {
			CFormVisualDocument* foundedVisualDocument = *foundedSourceForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

CValueForm* CFormVisualDocument::FindFormBySourceUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {
		std::set<CFormVisualDocument*>::iterator foundedSourceForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CFormVisualDocument* visualDoc) {
					CValueForm* valueForm = visualDoc->GetValueForm();
					wxASSERT(valueForm);
					ISourceDataObject* sourceObject = valueForm->GetSourceObject();
					if (sourceObject != nullptr) return formKey == sourceObject->GetGuid();
					return false;
				}
			);

		if (foundedSourceForm != s_createdDocFormArray.end()) {
			CFormVisualDocument* foundedVisualDocument = *foundedSourceForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

CFormVisualDocument* CFormVisualDocument::FindDocByUniqueKey(const CUniqueKey& formKey)
{
	for (auto& visualDocument : s_createdDocFormArray) {
		if (visualDocument != nullptr &&
			visualDocument->CompareFormKey(formKey))
		{
			return visualDocument;
		}
	}

	return nullptr;
}

bool CFormVisualDocument::UpdateFormUniqueKey(const CUniquePairKey& formKey)
{
	std::set<CFormVisualDocument*>::iterator foundedForm =
		std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
			[formKey](const CFormVisualDocument* visualDoc) {
				return visualDoc != nullptr &&
					visualDoc->GetFormKey().GetGuid() == formKey.GetGuid();
			}
		);

	if (foundedForm != s_createdDocFormArray.end()) {
		CFormVisualDocument* visualDocument = *foundedForm;
		wxASSERT(visualDocument);
		CValueForm* formValue = visualDocument->GetValueForm();
		wxASSERT(formValue);
		formValue->m_formKey = formKey;
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////

wxPrintout* CFormVisualEditView::OnCreatePrintout()
{
	wxWindow* focusedWindow = wxWindow::FindFocus();
	while (focusedWindow != nullptr) {

		IValueFrame* currentFrame = m_visualHost->GetObjectBase(focusedWindow);
		if (currentFrame != nullptr)
			return currentFrame->CreatePrintout();

		focusedWindow = focusedWindow->GetParent();
	}

	return nullptr;
}

bool CFormVisualEditView::OnCreate(CMetaDocument* doc, long flags)
{
	std::set<CFormVisualDocument*>::iterator foundedVisualDoc =

		std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
			[doc](const CFormVisualDocument* visualDoc) {
				return doc != nullptr &&
					doc == visualDoc;
			}
		);

	if (foundedVisualDoc != s_createdDocFormArray.end()) {
		CValueForm* const valueForm = (*foundedVisualDoc)->GetValueForm();
		wxASSERT(valueForm);
		m_visualHost = new CVisualClientHost(*foundedVisualDoc, valueForm, m_viewFrame);
		return m_visualHost->CreateAndUpdateVisualHost();
	}

	return CMetaView::OnCreate(doc, flags);
}

void CFormVisualEditView::OnUpdate(wxView* sender, wxObject* hint)
{
	if (m_visualHost != nullptr)
		m_visualHost->UpdateForm();
}

bool CFormVisualEditView::OnClose(bool deleteWindow)
{
	if (!deleteWindow) {

		CMetaDocument const* doc = GetDocument();
		wxASSERT(doc);

		std::set<CFormVisualDocument*>::iterator foundedVisualDoc =

			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[doc](const CFormVisualDocument* visualDoc) {
					return doc != nullptr &&
						doc == visualDoc;
				}
			);

		if (foundedVisualDoc != s_createdDocFormArray.end()) {
			CValueForm* const valueForm = (*foundedVisualDoc)->GetValueForm();
			wxASSERT(valueForm);
			if (valueForm != nullptr && !valueForm->CloseDocForm())
				return false;
		}
	}

	//if (CDocMDIFrame::Get())
	//	Activate(false);

	if (deleteWindow) {
		GetFrame()->Destroy();
		SetFrame(nullptr);
	}

	return CMetaView::OnClose(deleteWindow);
}

void CFormVisualEditView::OnClosingDocument()
{
	if (m_visualHost != nullptr) {
		wxTheApp->ScheduleForDestruction(m_visualHost);
		m_visualHost = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CFormVisualEditView::~CFormVisualEditView()
{
	if (m_visualHost != nullptr && !wxTheApp->IsScheduledForDestruction(m_visualHost)) {
		wxTheApp->ScheduleForDestruction(m_visualHost);
	}
}
