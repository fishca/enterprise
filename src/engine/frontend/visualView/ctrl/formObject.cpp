////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : frame object
////////////////////////////////////////////////////////////////////////////

#include "form.h"
#include "backend/appData.h"
#include "backend/metaData.h"
#include "frontend/docView/docManager.h"
#include "backend/srcExplorer.h"
#include "backend/moduleManager/moduleManager.h"
#include "frontend/visualView/visualHost.h"

//*************************************************************************************************
//*                                    System attribute                                           *
//*************************************************************************************************

#include "toolBar.h"
#include "tableBox.h"

void CValueForm::BuildForm(const form_identifier_t& formType)
{
	m_formType = formType;

	if (m_sourceObject != nullptr) {

		CValue* prevSrcData = nullptr;

		CValueToolbar* mainToolBar =
			wxDynamicCast(
				CValueForm::CreateControl(wxT("toolbar")), CValueToolbar
			);

		mainToolBar->SetControlName(wxT("mainToolbar"));
		mainToolBar->SetActionSrc(FORM_ACTION);

		const IMetaObjectGenericData* metaObjectValue = m_sourceObject->GetSourceMetaObject();

		CValueTableBox* mainTableBox = nullptr;

		const CActionCollection& actionData = CValueForm::GetActionCollection(formType);
		for (unsigned int idx = 0; idx < actionData.GetCount(); idx++) {
			const action_identifier_t& action_id = actionData.GetID(idx);
			if (action_id != wxNOT_FOUND) {
				CValue* currSrcData = actionData.GetSourceDataByID(action_id);
				if (currSrcData != prevSrcData
					&& prevSrcData != nullptr) {
					CValueForm::CreateControl(wxT("toolSeparator"), mainToolBar);
				}
				CValueToolBarItem* toolBarItem =
					wxDynamicCast(
						CValueForm::CreateControl(wxT("tool"), mainToolBar), CValueToolBarItem
					);
				toolBarItem->SetControlName(mainToolBar->GetControlName() + wxT("_") + actionData.GetNameByID(action_id));
				toolBarItem->SetCaption(actionData.GetCaptionByID(action_id));
				toolBarItem->SetAction(action_id);
				prevSrcData = currSrcData;
			}
			else {
				CValueForm::CreateControl(wxT("toolSeparator"), mainToolBar);
			}
		}

		const CSourceExplorer& sourceExplorer = m_sourceObject->GetSourceExplorer();
		if (sourceExplorer.IsTableSection()) {

			mainTableBox =
				wxDynamicCast(
					CValueForm::CreateControl(wxT("tablebox")), CValueTableBox
				);

			mainTableBox->SetControlName(sourceExplorer.GetSourceName());
			mainTableBox->SetSource(sourceExplorer.GetSourceId());
		}

		for (unsigned int idx = 0; idx < sourceExplorer.GetHelperCount(); idx++) {

			const CSourceExplorer& nextSourceExplorer = sourceExplorer.GetHelper(idx);

			if (sourceExplorer.IsTableSection()) {
				CValueTableBoxColumn* tableBoxColumn =
					wxDynamicCast(
						CValueForm::CreateControl(wxT("tableboxColumn"), mainTableBox), CValueTableBoxColumn
					);
				tableBoxColumn->SetControlName(mainTableBox->GetControlName() + wxT("_") + nextSourceExplorer.GetSourceName());
				tableBoxColumn->SetVisibleColumn(nextSourceExplorer.IsVisible() || sourceExplorer.GetHelperCount() == 1);
				tableBoxColumn->SetSource(nextSourceExplorer.GetSourceId());
			}
			else
			{
				prevSrcData = nullptr;

				if (nextSourceExplorer.IsTableSection()) {

					CValueToolbar* toolBar =
						wxDynamicCast(
							CValueForm::CreateControl(wxT("toolbar")), CValueToolbar
						);

					toolBar->SetControlName(wxT("toolbar_") + nextSourceExplorer.GetSourceName());

					CValueTableBox* tableBox =
						wxDynamicCast(
							CValueForm::CreateControl(wxT("tablebox")), CValueTableBox
						);

					tableBox->SetControlName(nextSourceExplorer.GetSourceName());
					tableBox->SetSource(nextSourceExplorer.GetSourceId());

					toolBar->SetActionSrc(tableBox->GetControlID());

					CActionCollection actionData = tableBox->GetActionCollection(formType);
					for (unsigned int idx = 0; idx < actionData.GetCount(); idx++) {
						const action_identifier_t& action_id = actionData.GetID(idx);
						if (action_id != wxNOT_FOUND) {
							CValue* currSrcData = actionData.GetSourceDataByID(action_id);
							if (currSrcData != prevSrcData
								&& prevSrcData != nullptr) {
								CValueForm::CreateControl(wxT("toolSeparator"), toolBar);
							}
							CValueToolBarItem* toolBarItem =
								wxDynamicCast(
									CValueForm::CreateControl(wxT("tool"), toolBar), CValueToolBarItem
								);
							toolBarItem->SetControlName(toolBar->GetControlName() + wxT("_") + actionData.GetNameByID(action_id));
							toolBarItem->SetCaption(actionData.GetCaptionByID(action_id));
							toolBarItem->SetAction(action_id);
							prevSrcData = currSrcData;
						}
						else {
							CValueForm::CreateControl(wxT("toolSeparator"), toolBar);
						}
					}

					for (unsigned int col = 0; col < nextSourceExplorer.GetHelperCount(); col++) {
						const CSourceExplorer& colSourceExplorer = nextSourceExplorer.GetHelper(col);

						CValueTableBoxColumn* tableBoxColumn =
							wxDynamicCast(
								CValueForm::CreateControl(wxT("tableboxColumn"), tableBox), CValueTableBoxColumn
							);
						tableBoxColumn->SetControlName(tableBox->GetControlName() + wxT("_") + colSourceExplorer.GetSourceName());
						//tableBoxColumn->SetCaption(colSourceExplorer.GetSourceSynonym());
						tableBoxColumn->SetVisibleColumn(colSourceExplorer.IsVisible()
							|| nextSourceExplorer.GetHelperCount() == 1);
						tableBoxColumn->SetSource(colSourceExplorer.GetSourceId());
					}
				}
				else {
					if (nextSourceExplorer.ContainType(eValueTypes::TYPE_BOOLEAN)
						&& nextSourceExplorer.GetClsidList().size() == 1) {
						CValueCheckbox* checkbox =
							wxDynamicCast(
								CValueForm::CreateControl(wxT("checkbox")), CValueCheckbox
							);
						checkbox->SetControlName(nextSourceExplorer.GetSourceName());
						//checkbox->SetCaption(nextSourceExplorer.GetSourceSynonym());
						checkbox->EnableWindow(nextSourceExplorer.IsEnabled());
						checkbox->VisibleWindow(nextSourceExplorer.IsVisible());
						checkbox->SetSource(nextSourceExplorer.GetSourceId());
					}
					else {

						bool selButton = !nextSourceExplorer.ContainType(eValueTypes::TYPE_BOOLEAN) &&
							!nextSourceExplorer.ContainType(eValueTypes::TYPE_NUMBER) &&
							//!nextSourceExplorer.ContainType(eValueTypes::TYPE_DATE) &&
							!nextSourceExplorer.ContainType(eValueTypes::TYPE_STRING);

						if (nextSourceExplorer.GetClsidList().size() != 1)
							selButton = true;

						CValueTextCtrl* textCtrl =
							wxDynamicCast(
								CValueForm::CreateControl(wxT("textctrl")), CValueTextCtrl
							);
						textCtrl->SetControlName(nextSourceExplorer.GetSourceName());
						//textCtrl->SetCaption(nextSourceExplorer.GetSourceSynonym());
						textCtrl->EnableWindow(nextSourceExplorer.IsEnabled());
						textCtrl->VisibleWindow(nextSourceExplorer.IsVisible());
						textCtrl->SetSource(nextSourceExplorer.GetSourceId());

						textCtrl->SetSelectButton(selButton);
						textCtrl->SetOpenButton(false);
						textCtrl->SetClearButton(nextSourceExplorer.IsEnabled());
					}
				}
			}
		}
	}
	else {

		CValueToolbar* mainToolBar =
			wxDynamicCast(
				CValueForm::CreateControl(wxT("toolbar")), CValueToolbar
			);

		mainToolBar->SetControlName(wxT("mainToolbar"));
		mainToolBar->SetActionSrc(FORM_ACTION);

		CValueTableBox* mainTableBox = nullptr;

		const CActionCollection& actionData = CValueForm::GetActionCollection(formType);
		for (unsigned int idx = 0; idx < actionData.GetCount(); idx++) {

			const action_identifier_t& id = actionData.GetID(idx);

			if (id != wxNOT_FOUND) {
				CValueToolBarItem* toolBarItem =
					wxDynamicCast(
						CValueForm::CreateControl(wxT("tool"), mainToolBar), CValueToolBarItem
					);
				toolBarItem->SetControlName(mainToolBar->GetControlName() + wxT("_") + actionData.GetNameByID(id));
				toolBarItem->SetCaption(actionData.GetCaptionByID(id));
				toolBarItem->SetAction(id);
			}
			else {
				CValueForm::CreateControl(wxT("toolSeparator"), mainToolBar);
			}
		}
	}
}

void CValueForm::InitializeForm(const IMetaObjectForm* creator,
	IControlFrame* ownerControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	if (ownerControl != nullptr) ownerControl->ControlIncrRef();
	if (m_controlOwner != nullptr) m_controlOwner->ControlDecrRef();

	if (srcObject != nullptr) srcObject->SourceIncrRef();
	if (m_sourceObject != nullptr) m_sourceObject->SourceDecrRef();

	m_controlOwner = ownerControl;
	m_sourceObject = srcObject;
	m_metaFormObject = creator;

	m_formKey = CreateFormUniqueKey(ownerControl, srcObject, formGuid);

	if (creator != nullptr)
		m_formType = creator->GetTypeForm();

	//SetReadOnly(readOnly);
}

bool CValueForm::InitializeFormModule()
{
	if (m_metaFormObject != nullptr) {

		IMetaData* metaData = m_metaFormObject->GetMetaData();
		wxASSERT(metaData);
		IModuleManager* moduleManager = metaData->GetModuleManager();
		wxASSERT(moduleManager);

		IModuleDataObject* sourceObjectValue =
			dynamic_cast<IModuleDataObject*>(m_sourceObject);

		if (m_compileModule == nullptr) {
			m_compileModule = new CCompileModule(m_metaFormObject);
			m_compileModule->SetParent(
				sourceObjectValue != nullptr ? sourceObjectValue->GetCompileModule() :
				moduleManager->GetCompileModule()
			);
			m_compileModule->AddContextVariable(thisForm, this);
		}

		if (!appData->DesignerMode()) {

			try {
				m_compileModule->Compile();
			}
			catch (const CBackendException*) {

				if (!appData->DesignerMode())
					throw(std::exception());

				return false;
			};

			if (m_procUnit == nullptr) {
				m_procUnit = new CProcUnit();
				m_procUnit->SetParent(
					sourceObjectValue != nullptr ? sourceObjectValue->GetProcUnit() :
					moduleManager->GetProcUnit()
				);
			}

			m_procUnit->Execute(m_compileModule->m_cByteCode, true);
		}
	}

#pragma region _control_guard_

	struct CControlGuard {

		static bool Initialize(IValueFrame* controlParent) {
			for (unsigned int idx = controlParent->GetChildCount(); idx > 0; idx--) {
				if (!Initialize(controlParent->GetChild(idx - 1)))
					return false;
			}
			return controlParent->InitializeControl();
		}
	};

	return CControlGuard::Initialize(this);

#pragma endregion 
}

#include "backend/system/value/valueType.h"

void CValueForm::NotifyCreate(const CValue& vCreated)
{
	CValueForm* ownerForm = m_controlOwner != nullptr ?
		m_controlOwner->GetOwnerForm() : nullptr;

	if (ownerForm != nullptr) {

		ownerForm->m_createdValue = vCreated;
		ownerForm->m_changedValue = wxEmptyValue;

		ownerForm->UpdateForm();
	}

	CValueForm::UpdateForm();
	CValueForm::Modify(false);
}

void CValueForm::NotifyChange(const CValue& vChanged)
{
	CValueForm* ownerForm = m_controlOwner != nullptr ?
		m_controlOwner->GetOwnerForm() : nullptr;

	if (ownerForm != nullptr) {

		ownerForm->m_createdValue = wxEmptyValue;
		ownerForm->m_changedValue = vChanged;

		ownerForm->UpdateForm();
	}

	CValueForm::UpdateForm();
	CValueForm::Modify(false);
}

void CValueForm::NotifyDelete(const CValue& vChanged)
{
	CValueForm* ownerForm = m_controlOwner != nullptr ?
		m_controlOwner->GetOwnerForm() : nullptr;

	if (ownerForm != nullptr) {

		ownerForm->m_createdValue = wxEmptyValue;
		ownerForm->m_changedValue = wxEmptyValue;

		ownerForm->UpdateForm();
	}

	CValueForm::CloseForm(true);
}

void CValueForm::NotifyChoice(CValue& vSelected)
{
	ChoiceDocForm(vSelected);

	if (m_closeOnChoice)
		CValueForm::CloseForm();
}

#include "backend/system/systemManager.h"

CValue CValueForm::CreateControl(const CValueType* clsControl, const CValue& vControl)
{
	if (appData->DesignerMode())
		return CValue();

	if (!CValue::IsRegisterCtor(clsControl->GetString(), eCtorObjectType::eCtorObjectType_object_control)) {
		CSystemFunction::Raise(_("Ñlass does not belong to control!"));
	}

	//get parent obj
	IValueFrame* parentControl = nullptr;

	if (!vControl.IsEmpty())
		parentControl = CastValue<IValueFrame>(vControl);
	else
		parentControl = this;

	return CValueForm::CreateControl(
		clsControl->GetString(),
		parentControl
	);
}

CValue CValueForm::FindControl(const CValue& vControl)
{
	IValueFrame* foundedControl = FindControlByName(vControl.GetString());
	if (foundedControl != nullptr)
		return foundedControl;
	return CValue();
}

void CValueForm::RemoveControl(const CValue& vControl)
{
	if (appData->DesignerMode())
		return;

	//get parent obj
	IValueControl* currentControl =
		CastValue<IValueControl>(vControl);

	wxASSERT(currentControl);
	RemoveControl(currentControl);
}

//*************************************************************************************************
//*                                              Events                                           *
//*************************************************************************************************

void CValueForm::ShowForm(IBackendMetaDocument* doc, bool createContext)
{
	CMetaDocument* docParent = wxDynamicCast(doc, CMetaDocument);

	if (CBackendException::IsEvalMode())
		return;

	CVisualDocument* const ownerDocForm = GetVisualDocument();

	if (ownerDocForm != nullptr) {
		ActivateForm();
		return;
	}

	if (m_controlOwner != nullptr &&
		doc == nullptr) {
		docParent = m_controlOwner->GetVisualDocument();
	}

	if (!createContext || !appData->DesignerMode()) {
		CreateDocForm(docParent, createContext);
	}
}

void CValueForm::UpdateForm()
{
	if (CBackendException::IsEvalMode())
		return;

	CVisualDocument* const ownerDocForm = GetVisualDocument();

	if (ownerDocForm != nullptr) {

		CVisualHost* visualView = ownerDocForm->GetFirstView() ?
			ownerDocForm->GetFirstView()->GetVisualHost() : nullptr;

		if (visualView != nullptr) {
			visualView->Freeze();
			visualView->UpdateVisualHost();
			visualView->Thaw();
		}
	}
}

bool CValueForm::CloseForm(bool force)
{
	if (CBackendException::IsEvalMode())
		return false;

	if (!appData->DesignerMode()) {
		if (!force && !CloseDocForm()) {
			return false;
		}
	}

	CVisualDocument* const ownerDocForm = GetVisualDocument();

	if (ownerDocForm != nullptr) {
		return ownerDocForm->DeleteAllViews();
	}

	return true;
}

void CValueForm::HelpForm()
{
	wxMessageBox(
		_("Help will appear here sometime, but not today.")
	);
}

#include "frontend/win/dlgs/formEditor.h"

void CValueForm::ChangeForm()
{
	CDialogFormEditor* formEditor = new CDialogFormEditor(this);
	formEditor->ShowModal();
}

#include "frontend/win/dlgs/generation.h"

bool CValueForm::GenerateForm(IRecordDataObjectRef* obj) const
{
	IMetaObjectRecordDataMutableRef* metaObject = obj->GetMetaObject();
	wxASSERT(metaObject);
	IMetaData* metaData = metaObject->GetMetaData();
	wxASSERT(metaData);

	CDialogGeneration* selectDataType = new CDialogGeneration(metaData, metaObject->GetGenerationDescription());

	meta_identifier_t sel_id = 0;
	if (selectDataType->ShowModal(sel_id)) {
		IMetaObjectRecordDataMutableRef* meta = nullptr;
		if (metaData->GetMetaObject(meta, sel_id)) {
			IRecordDataObjectRef* genObj = meta->CreateObjectValue(obj, true);
			if (genObj != nullptr) {
				genObj->ShowFormValue();
				selectDataType->Destroy();
				return true;
			}

		}
		selectDataType->Destroy();
		return false;
	}
	selectDataType->Destroy();
	return false;
}

//**********************************************************************************
//*                                   Other                                        *
//**********************************************************************************

IValueFrame* CValueForm::CreateControl(const wxString& clsControl, IValueFrame* control)
{
	//get parent obj
	IValueFrame* parentControl = nullptr;

	if (control != nullptr)
		parentControl = control;
	else
		parentControl = this;

	// ademas, el objeto se insertara a continuacion del objeto seleccionado
	IValueFrame* newControl = CValueForm::CreateObject(clsControl, parentControl);
	wxASSERT(newControl);
	if (!CBackendException::IsEvalMode()) {
		CVisualDocument* const ownerDocForm = GetVisualDocument();
		if (ownerDocForm != nullptr) {
			CVisualHost* visualView = ownerDocForm->GetFirstView() ?
				ownerDocForm->GetFirstView()->GetVisualHost() : nullptr;
			visualView->CreateControl(newControl);
		}
	}

	m_formCollectionControl->PrepareNames();

	//return value 
	if (newControl->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
		return newControl->GetChild(0);

	return newControl;
}

void CValueForm::RemoveControl(IValueFrame* control)
{
	//get parent obj
	IValueFrame* currentControl = control;
	wxASSERT(currentControl);
	if (!CBackendException::IsEvalMode()) {
		CVisualDocument* const ownerDocForm = GetVisualDocument();
		if (ownerDocForm != nullptr) {
			CVisualHost* visualView = ownerDocForm->GetFirstView() ?
				ownerDocForm->GetFirstView()->GetVisualHost() : nullptr;
			visualView->RemoveControl(currentControl);
		}
	}

	IValueFrame* parentControl = currentControl->GetParent();

	if (parentControl->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		IValueFrame* parentOwner = parentControl->GetParent();
		if (parentOwner != nullptr) {
			parentOwner->RemoveChild(parentControl);
		}
		parentControl->SetParent(nullptr);
		parentControl->RemoveChild(currentControl);
		parentControl->DecrRef();

		currentControl->SetParent(nullptr);
		currentControl->DecrRef();
	}
	else {
		IValueFrame* parentOwner = currentControl->GetParent();
		if (parentOwner != nullptr) {
			parentOwner->RemoveChild(currentControl);
		}
		currentControl->SetParent(nullptr);
		currentControl->DecrRef();
	}

	m_formCollectionControl->PrepareNames();
}

void CValueForm::AttachIdleHandler(const wxString& procedureName, int interval, bool single)
{
	if (appData->DesignerMode())
		return;

	for (unsigned int i = 0; i < procedureName.length(); i++) {
		if (!((procedureName[i] >= 'A' && procedureName[i] <= 'Z') || (procedureName[i] >= 'a' && procedureName[i] <= 'z') ||
			(procedureName[i] >= 'À' && procedureName[i] <= 'ß') || (procedureName[i] >= 'à' && procedureName[i] <= 'ÿ') ||
			(procedureName[i] >= '0' && procedureName[i] <= '9')))
		{
			CBackendException::Error(_("Procedure can enter only numbers, letters and the symbol \"_\""));
			return;
		}
	}

	if (m_procUnit != nullptr && m_procUnit->FindMethod(procedureName, true)) {
		auto& it = m_idleHandlerArray.find(procedureName);
		if (it == m_idleHandlerArray.end()) {
			wxTimer* timer = new wxTimer();
			timer->Bind(wxEVT_TIMER, &CValueForm::OnIdleHandler, this);
			if (timer->Start(interval * 1000, single)) {
				m_idleHandlerArray.insert_or_assign(procedureName, timer);
			}
		}
	}
}

void CValueForm::DetachIdleHandler(const wxString& procedureName)
{
	if (appData->DesignerMode())
		return;

	for (unsigned int i = 0; i < procedureName.length(); i++) {
		if (!((procedureName[i] >= 'A' && procedureName[i] <= 'Z') || (procedureName[i] >= 'a' && procedureName[i] <= 'z') ||
			(procedureName[i] >= 'À' && procedureName[i] <= 'ß') || (procedureName[i] >= 'à' && procedureName[i] <= 'ÿ') ||
			(procedureName[i] >= '0' && procedureName[i] <= '9')))
		{
			CBackendException::Error(_("Procedure can enter only numbers, letters and the symbol \"_\""));
			return;
		}
	}

	if (m_procUnit != nullptr && m_procUnit->FindMethod(procedureName, true)) {
		auto& it = m_idleHandlerArray.find(procedureName);
		if (it != m_idleHandlerArray.end()) {
			wxTimer* timer = it->second;
			m_idleHandlerArray.erase(it);
			if (timer != nullptr && timer->IsRunning()) {
				timer->Stop();
			}
			timer->Unbind(wxEVT_TIMER, &CValueForm::OnIdleHandler, this);
			delete timer;
		}
	}
}

void CValueForm::ClearRecursive(IValueFrame* control)
{
	for (unsigned int idx = control->GetChildCount(); idx > 0; idx--) {
		IValueFrame* controlChild = control->GetChild(idx - 1);
		ClearRecursive(controlChild);
		if (controlChild != nullptr) {
			controlChild->DecrRef();
		}
	}
}