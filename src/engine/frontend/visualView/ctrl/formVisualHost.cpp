////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : form host
////////////////////////////////////////////////////////////////////////////

#include "form.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "frontend/visualView/visualHostClient.h"

#include "backend/appData.h"

#define runFlag 0x000000
#define demoFlag 0x000001

//////////////////////////////////////////////////////////////

void CValueForm::Modify(bool modify) 
{
	if (IsShown()) {
		GetVisualDocument()->Modify(modify);
	}
	m_formModified = modify;
}

CFormVisualDocument* CValueForm::GetVisualDocument() const
{
	return CFormVisualDocument::FindDocByUniqueKey(m_formKey);
}

IValueMetaObjectGenericData* CValueForm::GetMetaObject() const
{
	return m_sourceObject != nullptr ?
		m_sourceObject->GetSourceMetaObject() : nullptr;
}

//////////////////////////////////////////////////////////////

wxString CValueForm::GetControlTitle() const
{
	if (m_propertyTitle->IsEmptyProperty()) {

		const IValueMetaObjectGenericData* metaSource = GetMetaObject();
		if (metaSource != nullptr) return metaSource->GetSynonym();

		const IValueMetaObjectForm* metaForm = GetFormMetaObject();
		if (metaForm != nullptr) return metaForm->GetSynonym();
	}

	return m_propertyTitle->GetValueAsTranslateString();
}

//////////////////////////////////////////////////////////////

#include "frontend/docView/docManager.h"

bool CValueForm::CreateDocForm(CMetaDocument* docParent, bool createContext)
{
	CFormVisualDocument* const visualFoundedDoc = GetVisualDocument();
	if (visualFoundedDoc != nullptr) {
		ActivateForm();
		return true;
	}

	if (createContext) {
		ISourceDataObject* srcData = GetSourceObject();
		if (srcData != nullptr && !srcData->IsNewObject()) {
			CFormVisualDocument* foundedVisualDocument =
				CFormVisualDocument::FindDocByUniqueKey(m_formKey);
			if (foundedVisualDocument != nullptr) {
				foundedVisualDocument->Activate();
				return true;
			}
		}
		else if (srcData == nullptr) {
			CFormVisualDocument* foundedVisualDocument =
				CFormVisualDocument::FindDocByUniqueKey(m_formKey);
			if (foundedVisualDocument != nullptr) {
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

	CFormVisualDocument* visualCreatedDoc = createContext ?
		new CFormVisualDocument(this) :
		new CFormVisualDemoDocument(this);

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
	CFormVisualDocument* const ownerDocForm = GetVisualDocument();
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
	CFormVisualDocument* const visualDoc = GetVisualDocument();

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
