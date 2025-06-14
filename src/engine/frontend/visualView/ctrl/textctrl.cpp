#include "widgets.h"
#include "frontend/win/ctrls/textEditor.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueTextCtrl, IValueWindow)

//****************************************************************************

#include "form.h"
#include "backend/metaData.h"
#include "backend/objCtor.h"

bool CValueTextCtrl::GetChoiceForm(CPropertyList* property)
{
	property->AppendItem(_("default"), wxNOT_FOUND, wxEmptyValue);
	IMetaData* metaData = GetMetaData();
	if (metaData != nullptr) {
		IMetaObjectRecordDataRef* metaObject = nullptr;
		if (!m_propertySource->IsEmptyProperty()) {
			const IMetaObjectGenericData* metaObjectValue =
				m_formOwner->GetMetaObject();
			if (metaObjectValue != nullptr) {
				IMetaObjectAttribute* metaAttribute = wxDynamicCast(
					metaObjectValue->FindMetaObjectByID(m_propertySource->GetValueAsSource()), IMetaObjectAttribute
				);
				wxASSERT(metaAttribute);
				IMetaValueTypeCtor* so = metaData->GetTypeCtor(metaAttribute->GetFirstClsid());
				if (so != nullptr) {
					metaObject = wxDynamicCast(so->GetMetaObject(), IMetaObjectRecordDataRef);
				}
			}
		}
		else {
			IMetaValueTypeCtor* so = metaData->GetTypeCtor(ITypeControlFactory::GetFirstClsid());
			if (so != nullptr) {
				metaObject = wxDynamicCast(so->GetMetaObject(), IMetaObjectRecordDataRef);
			}
		}

		if (metaObject != nullptr) {
			for (auto form : metaObject->GetObjectForms()) {
				property->AppendItem(
					form->GetSynonym(),
					form->GetMetaID(),
					form
				);
			}
		}
	}

	return true;
}

ISourceObject* CValueTextCtrl::GetSourceObject() const
{
	return m_formOwner ? m_formOwner->GetSourceObject()
		: nullptr;
}

//****************************************************************************
//*                              TextCtrl                                    *
//****************************************************************************

CValueTextCtrl::CValueTextCtrl() :
	IValueWindow(), ITypeControlFactory(), m_textModified(false)
{
}

IMetaData* CValueTextCtrl::GetMetaData() const
{
	return m_formOwner ?
		m_formOwner->GetMetaData() : nullptr;
}

wxObject* CValueTextCtrl::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxTextContainerCtrl* textEditor = new wxTextContainerCtrl(wxparent, wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize);

	if (!m_propertySource->IsEmptyProperty()) {
		ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
		if (srcObject != nullptr) {
			srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
		}
	}
	else {
		m_selValue = ITypeControlFactory::CreateValue();
	}

	return textEditor;
}

void CValueTextCtrl::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool firstСreated)
{
}

#include "backend/appData.h"

void CValueTextCtrl::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxTextContainerCtrl* textEditor = dynamic_cast<wxTextContainerCtrl*>(wxobject);

	if (textEditor != nullptr) {
		wxString textCaption = wxEmptyString;

		if (!m_propertySource->IsEmptyProperty()) {
			const IMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
			if (metaObject != nullptr) textCaption = metaObject->GetSynonym() + wxT(":");
		}

		textEditor->SetTextLabel(m_propertyCaption->IsEmptyProperty() ?
			textCaption : m_propertyCaption->GetValueAsString());

		if (!m_propertySource->IsEmptyProperty()) {
			ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
			if (srcObject != nullptr) {
				srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
			}
		}

		if (!appData->DesignerMode()) {
			textEditor->SetTextValue(m_selValue.GetString());
		}

		textEditor->SetPasswordMode(m_propertyPasswordMode->GetValueAsBoolean());
		textEditor->SetMultilineMode(m_propertyMultilineMode->GetValueAsBoolean());
		textEditor->SetTextEditMode(m_propertyTexteditMode->GetValueAsBoolean());

		if (!appData->DesignerMode()) {
			textEditor->SetButtonSelect(m_propertySelectButton->GetValueAsBoolean());
			textEditor->BindButtonSelect(&CValueTextCtrl::OnSelectButtonPressed, this);
			textEditor->SetButtonOpen(m_propertyOpenButton->GetValueAsBoolean());
			textEditor->BindButtonOpen(&CValueTextCtrl::OnOpenButtonPressed, this);
			textEditor->SetButtonClear(m_propertyClearButton->GetValueAsBoolean());
			textEditor->BindButtonClear(&CValueTextCtrl::OnClearButtonPressed, this);

			textEditor->BindTextEnter(&CValueTextCtrl::OnTextEnter, this);
			textEditor->BindTextUpdated(&CValueTextCtrl::OnTextUpdated, this);

			textEditor->BindKillFocus(&CValueTextCtrl::OnKillFocus, this);
		}
		else {
			textEditor->SetButtonSelect(m_propertySelectButton->GetValueAsBoolean());
			textEditor->SetButtonOpen(m_propertyOpenButton->GetValueAsBoolean());
			textEditor->SetButtonClear(m_propertyClearButton->GetValueAsBoolean());
		}
	}

	UpdateWindow(textEditor);
	UpdateLabelSize(textEditor);
}

void CValueTextCtrl::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
	wxTextContainerCtrl* textEditor = dynamic_cast<wxTextContainerCtrl*>(wxobject);

	if (textEditor != nullptr) {
		if (!appData->DesignerMode()) {
			textEditor->UnbindButtonSelect(&CValueTextCtrl::OnSelectButtonPressed, this);
			textEditor->UnbindButtonOpen(&CValueTextCtrl::OnOpenButtonPressed, this);
			textEditor->UnbindButtonClear(&CValueTextCtrl::OnClearButtonPressed, this);

			textEditor->UnbindTextEnter(&CValueTextCtrl::OnTextEnter, this);
			textEditor->UnbindTextUpdated(&CValueTextCtrl::OnTextUpdated, this);

			textEditor->UnbindKillFocus(&CValueTextCtrl::OnKillFocus, this);
		}
	}
}

//*******************************************************************
//*							 Control value	                        *
//*******************************************************************

bool CValueTextCtrl::GetControlValue(CValue& pvarControlVal) const
{
	const ISourceDataObject* sourceObject = m_formOwner->GetSourceObject();
	if (!m_propertySource->IsEmptyProperty() && sourceObject != nullptr) {
		return sourceObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), pvarControlVal);
	}

	pvarControlVal = m_selValue;
	return true;
}

bool CValueTextCtrl::SetControlValue(const CValue& varControlVal)
{
	ISourceDataObject* sourceObject = m_formOwner->GetSourceObject();
	if (!m_propertySource->IsEmptyProperty() && sourceObject != nullptr) {
		//IMetaObjectAttribute* metaObject = wxDynamicCast(GetMetaSource(), IMetaObjectAttribute);
		IMetaObjectAttribute* metaObject = m_propertySource->GetSourceAttributeObject();
		wxASSERT(metaObject);
		sourceObject->SetValueByMetaID(m_propertySource->GetValueAsSource(), varControlVal);
		m_selValue = metaObject->AdjustValue(varControlVal);
	}
	else {
		m_selValue = ITypeControlFactory::AdjustValue(varControlVal);
	}

	m_formOwner->RefreshForm();

	wxTextContainerCtrl* textEditor = dynamic_cast<wxTextContainerCtrl*>(GetWxObject());
	if (textEditor != nullptr) {
		textEditor->SetTextValue(m_selValue.GetString());
		textEditor->SetInsertionPointEnd();
	}

	return true;
}

//*******************************************************************
//*                            Data		                            *
//*******************************************************************

bool CValueTextCtrl::LoadData(CMemoryReader& reader)
{
	wxString caption; reader.r_stringZ(caption);
	m_propertyCaption->SetValue(caption);

	m_propertyPasswordMode->SetValue(reader.r_u8());
	m_propertyMultilineMode->SetValue(reader.r_u8());
	m_propertyTexteditMode->SetValue(reader.r_u8());

	m_propertySelectButton->SetValue(reader.r_u8());
	m_propertyOpenButton->SetValue(reader.r_u8());
	m_propertyClearButton->SetValue(reader.r_u8());

	m_propertyChoiceForm->SetValue(reader.r_s32());

	if (!m_propertySource->LoadData(reader))
		return false;

	//events
	m_eventOnChange->LoadData(reader);
	m_eventStartChoice->LoadData(reader);
	m_eventStartListChoice->LoadData(reader);
	m_eventClearing->LoadData(reader);
	m_eventOpening->LoadData(reader);
	m_eventChoiceProcessing->LoadData(reader);

	return IValueWindow::LoadData(reader);
}

bool CValueTextCtrl::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(m_propertyCaption->GetValueAsString());

	writer.w_u8(m_propertyPasswordMode->GetValueAsBoolean());
	writer.w_u8(m_propertyMultilineMode->GetValueAsBoolean());
	writer.w_u8(m_propertyTexteditMode->GetValueAsBoolean());

	writer.w_u8(m_propertySelectButton->GetValueAsBoolean());
	writer.w_u8(m_propertyOpenButton->GetValueAsBoolean());
	writer.w_u8(m_propertyClearButton->GetValueAsBoolean());

	writer.w_s32(m_propertyChoiceForm->GetValueAsInteger());

	if (!m_propertySource->SaveData(writer))
		return false;

	//events
	m_eventOnChange->SaveData(writer);
	m_eventStartChoice->SaveData(writer);
	m_eventStartListChoice->SaveData(writer);
	m_eventClearing->SaveData(writer);
	m_eventOpening->SaveData(writer);
	m_eventChoiceProcessing->SaveData(writer);

	return IValueWindow::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueTextCtrl, "textctrl", "widget", string_to_clsid("CT_TXTC"));