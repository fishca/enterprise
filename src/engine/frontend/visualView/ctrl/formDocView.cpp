////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : frame docview
////////////////////////////////////////////////////////////////////////////

#include "form.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "frontend/visualView/visualHost.h"
//#include "window/visualView/printout/formPrintOut.h"
#include "frontend/mainFrame/mainFrame.h"
#include "frontend/mainFrame/mainFrameChild.h"

#include "backend/appData.h"

static std::set<CVisualDocument*> s_createdDocFormArray = {};

//********************************************************************************************
//*                                  Visual Document & View                                  *
//********************************************************************************************

CVisualView* CVisualDocument::GetFirstView() const
{
	return wxDynamicCast(
		CMetaDocument::GetFirstView(), CVisualView
	);
}

const CUniqueKey& CVisualDocument::GetFormKey() const
{
	return m_valueForm->GetFormKey();
}

bool CVisualDocument::CompareFormKey(const CUniqueKey& formKey) const
{
	return m_valueForm->CompareFormKey(formKey);
}

CValueForm* CVisualDocument::GetValueForm() const
{
	return m_valueForm;
}

bool CVisualDocument::OnCreate(const wxString& path, long flags)
{
	const ISourceDataObject* sourceObject = m_valueForm->GetSourceObject();

	if (sourceObject != nullptr && !IsVisualDemonstrationDoc()) {
		const IMetaObjectGenericData* genericObject = sourceObject->GetSourceMetaObject();
		if (genericObject != nullptr) {
			CVisualDocument::SetIcon(genericObject->GetIcon());
			CVisualDocument::SetFilename(genericObject->GetFileName());
		}
	}
	else {
		const IMetaObjectForm* creator = m_valueForm->GetFormMetaObject();
		if (creator != nullptr) {
			CVisualDocument::SetIcon(creator->GetIcon());
		}
		CVisualDocument::SetFilename(creator->GetFileName());
	}

	CVisualDocument::SetTitle(m_valueForm->GetCaption());

	if (IsVisualDemonstrationDoc()) m_childDoc = false;

	return CMetaDocument::OnCreate(path, flags);
}

bool CVisualDocument::OnCloseDocument()
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

bool CVisualDocument::IsCloseOnOwnerClose() const
{
	if (m_valueForm != nullptr)
		return m_valueForm->IsCloseOnOwnerClose();
	return true;
}

void CVisualDocument::Modify(bool modify)
{
	if (m_valueForm != nullptr)
		m_valueForm->m_formModified = modify;

	if (modify != m_documentModified) {

		m_documentModified = modify;

		// Allow views to append asterix to the title
		CVisualView* view = GetFirstView();
		if (view != nullptr) view->OnChangeFilename();
	}
}

#include "backend/system/systemManager.h"

bool CVisualDocument::Save()
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
		CVisualDocument::Modify(false);
		return true;
	}

	return true;
}

void CVisualDocument::SetDocParent(CMetaDocument* docParent)
{
	CMetaDocument::SetDocParent(docParent);

	if (docParent == nullptr &&
		(m_valueForm != nullptr && m_valueForm->m_controlOwner != nullptr)) {

		m_valueForm->m_controlOwner->ControlDecrRef();
		m_valueForm->m_controlOwner = nullptr;
	}
}

CMetaView* CVisualDocument::DoCreateView()
{
	return new CVisualView();
}

/////////////////////////////////////////////////////////////////////////////////////////////

CVisualDocument::CVisualDocument(CValueForm* valueForm)
	: m_valueForm(valueForm) {

	if (m_valueForm != nullptr) {

		CVisualDocument::SetCommandProcessor(new CVisualCommandProcessor);
		CVisualDocument::SetMetaObject(nullptr);
	}

	s_createdDocFormArray.insert(this);
}

CVisualDocument::~CVisualDocument()
{
	s_createdDocFormArray.erase(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////

IMetaData* CVisualDocument::GetMetaData() const
{
	return m_valueForm != nullptr ?
		m_valueForm->GetMetaData() : nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#define runFlag 0x000000
#define demoFlag 0x000001

wxPrintout* CVisualView::OnCreatePrintout() { return nullptr; }

bool CVisualView::OnCreate(CMetaDocument* doc, long flags)
{
	std::set<CVisualDocument*>::iterator foundedVisualDoc =

		std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
			[doc](const CVisualDocument* visualDoc) {
				return doc != nullptr &&
					doc == visualDoc;
			}
		);

	if (foundedVisualDoc != s_createdDocFormArray.end()) {
		CValueForm* const valueForm = (*foundedVisualDoc)->GetValueForm();
		wxASSERT(valueForm);
		m_visualHost = new CVisualHost(*foundedVisualDoc, valueForm, m_viewFrame);
		return m_visualHost->CreateAndUpdateVisualHost();
	}

	return CMetaView::OnCreate(doc, flags);
}

void CVisualView::OnUpdate(wxView* sender, wxObject* hint)
{
	if (m_visualHost != nullptr)
		m_visualHost->UpdateForm();
}

bool CVisualView::OnClose(bool deleteWindow)
{
	if (!deleteWindow) {

		CMetaDocument const* doc = GetDocument();
		wxASSERT(doc);

		std::set<CVisualDocument*>::iterator foundedVisualDoc =

			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[doc](const CVisualDocument* visualDoc) {
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

void CVisualView::OnClosingDocument()
{
	if (m_visualHost != nullptr) {
		wxTheApp->ScheduleForDestruction(m_visualHost);
		m_visualHost = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CVisualView::~CVisualView()
{
	if (m_visualHost != nullptr && !wxTheApp->IsScheduledForDestruction(m_visualHost)) {
		wxTheApp->ScheduleForDestruction(m_visualHost);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

CUniqueKey CValueForm::CreateFormUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formKey)
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

CValueForm* CValueForm::FindFormByUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formKey)
{
	return FindFormByUniqueKey(
		CreateFormUniqueKey(ownerControl, sourceObject, formKey)
	);
}

CValueForm* CValueForm::FindFormByUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {

		std::set<CVisualDocument*>::iterator foundedForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CVisualDocument* visualDoc) {
					return visualDoc->CompareFormKey(formKey);
				}
			);

		if (foundedForm != s_createdDocFormArray.end()) {
			CVisualDocument* foundedVisualDocument = *foundedForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

CValueForm* CValueForm::FindFormByControlUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {
		std::set<CVisualDocument*>::iterator foundedSourceForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CVisualDocument* visualDoc) {
					wxASSERT(visualDoc);
					CValueForm* valueForm = visualDoc->GetValueForm();
					wxASSERT(valueForm);
					IValueFrame* ownerControl = valueForm->GetOwnerControl();
					if (ownerControl != nullptr) return formKey == ownerControl->GetControlGuid();
					return false;
				}
			);

		if (foundedSourceForm != s_createdDocFormArray.end()) {
			CVisualDocument* foundedVisualDocument = *foundedSourceForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

CValueForm* CValueForm::FindFormBySourceUniqueKey(const CUniqueKey& formKey)
{
	if (formKey.isValid()) {
		std::set<CVisualDocument*>::iterator foundedSourceForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CVisualDocument* visualDoc) {
					CValueForm* valueForm = visualDoc->GetValueForm();
					wxASSERT(valueForm);
					ISourceDataObject* sourceObject = valueForm->GetSourceObject();
					if (sourceObject != nullptr) return formKey == sourceObject->GetGuid();
					return false;
				}
			);

		if (foundedSourceForm != s_createdDocFormArray.end()) {
			CVisualDocument* foundedVisualDocument = *foundedSourceForm;
			wxASSERT(foundedVisualDocument);
			return foundedVisualDocument->GetValueForm();
		}
	}

	return nullptr;
}

bool CValueForm::UpdateFormUniqueKey(const CUniquePairKey& formKey)
{
	std::set<CVisualDocument*>::iterator foundedForm =
		std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
			[formKey](const CVisualDocument* visualDoc) {
				return visualDoc != nullptr &&
					visualDoc->GetFormKey().GetGuid() == formKey.GetGuid();
			}
		);

	if (foundedForm != s_createdDocFormArray.end()) {
		CVisualDocument* visualDocument = *foundedForm;
		wxASSERT(visualDocument);
		CValueForm* formValue = visualDocument->GetValueForm();
		wxASSERT(formValue);
		formValue->m_formKey = formKey;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////

CVisualDocument* CValueForm::GetVisualDocument() const
{
	for (auto& visualDocument : s_createdDocFormArray) {
		if (visualDocument != nullptr &&
			visualDocument->CompareFormKey(m_formKey))
		{
			return visualDocument;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////

IMetaObjectGenericData* CValueForm::GetMetaObject() const
{
	return m_sourceObject != nullptr ?
		m_sourceObject->GetSourceMetaObject() : nullptr;
}

//////////////////////////////////////////////////////////////

wxString CValueForm::GetControlTitle() const
{
	if (m_propertyTitle->IsEmptyProperty()) {

		const IMetaObjectGenericData* metaSource = GetMetaObject();
		if (metaSource != nullptr) return metaSource->GetSynonym();

		const IMetaObjectForm* metaForm = GetFormMetaObject();
		if (metaForm != nullptr) return metaForm->GetSynonym();
	}

	return m_propertyTitle->GetValueAsTranslateString();
}

//////////////////////////////////////////////////////////////

#include "frontend/docView/docManager.h"

bool CValueForm::CreateDocForm(CMetaDocument* docParent, bool createContext)
{
	CVisualDocument* const visualFoundedDoc = GetVisualDocument();
	if (visualFoundedDoc != nullptr) {
		ActivateForm();
		return true;
	}

	if (createContext) {

		const CUniqueKey& formKey = m_formKey;

		std::set<CVisualDocument*>::iterator foundedForm =
			std::find_if(s_createdDocFormArray.begin(), s_createdDocFormArray.end(),
				[formKey](const CVisualDocument* visualDoc) {
					return visualDoc->CompareFormKey(formKey);
				}
			);

		ISourceDataObject* srcData = GetSourceObject();

		if (srcData != nullptr && !srcData->IsNewObject()) {
			if (foundedForm != s_createdDocFormArray.end()) {
				CVisualDocument* foundedVisualDocument = *foundedForm;
				wxASSERT(foundedVisualDocument);
				foundedVisualDocument->Activate();
				return true;
			}
		}
		else if (srcData == nullptr) {
			if (foundedForm != s_createdDocFormArray.end()) {
				CVisualDocument* foundedVisualDocument = *foundedForm;
				wxASSERT(foundedVisualDocument);
				foundedVisualDocument->Activate();
				return true;
			}
		}
	}

#pragma region __value_ref_guard_h__

	class CValueFormControlGuard {
	public:
		CValueFormControlGuard(CValueForm* valueForm) : m_valueForm(valueForm) { valueForm->IncrRef(); }
		~CValueFormControlGuard() { m_valueForm->DecrRef(); }
	private:
		CValueForm* m_valueForm;
	};

	CValueFormControlGuard enter(this);

#pragma endregion

	if (createContext) {

		CValue bCancel = false;

		if (!CallAsEvent(wxT("beforeOpen"), bCancel))
			return false;

		if (bCancel.GetBoolean())
			return false;

		if (!CallAsEvent(wxT("onOpen")))
			return false;
	}

	CVisualDocument* visualCreatedDoc = createContext ?
		new CVisualDocument(this) :
		new CVisualDemoDocument(this);

	//if doc has parent - special delete!
	if (docParent != nullptr) {
		visualCreatedDoc->SetDocParent(docParent);
	}
	else if (docParent == nullptr) {
		docManager->AddDocument(visualCreatedDoc);
	}

	if (visualCreatedDoc->OnCreate(m_formKey, createContext ? runFlag : demoFlag)) {

		if (createContext)
			visualCreatedDoc->Modify(m_formModified);

		RefreshForm();
		return true;
	}

	// The document may be already destroyed, this happens if its view
	// creation fails as then the view being created is destroyed
	// triggering the destruction of the document as this first view is
	// also the last one. However if OnCreate() fails for any reason other
	// than view creation failure, the document is still alive and we need
	// to clean it up ourselves to avoid having a zombie document.

	visualCreatedDoc->DeleteAllViews();
	return false;
}

void CValueForm::ActivateDocForm()
{
	CVisualDocument* const ownerDocForm = GetVisualDocument();
	if (ownerDocForm != nullptr) {
		CallAsEvent(wxT("onReOpen"));
		ownerDocForm->Activate();
	}
}

void CValueForm::ChoiceDocForm(CValue& vSelected)
{
	if (m_controlOwner != nullptr) {
		CValueForm* ownerForm = m_controlOwner->GetOwnerForm();
		if (ownerForm != nullptr)
			ownerForm->CallAsEvent(wxT("choiceProcessing"), vSelected, GetValue());
		m_controlOwner->ChoiceProcessing(vSelected);
		if (ownerForm != nullptr)
			ownerForm->UpdateForm();
	}
}

void CValueForm::RefreshDocForm()
{
	if (!appData->DesignerMode()) {
		CallAsEvent(wxT("refreshDisplay"));
	}
}

bool CValueForm::CloseDocForm()
{
	CVisualDocument* const visualDoc = GetVisualDocument();

	if (visualDoc == nullptr)
		return false;

	if (!appData->DesignerMode() && !visualDoc->IsVisualDemonstrationDoc()) {

		CValue bCancel = false;
		CallAsEvent(wxT("beforeClose"), bCancel);

		if (bCancel.GetBoolean())
			return false;

		CallAsEvent(wxT("onClose"));
	}

	return true;
}
