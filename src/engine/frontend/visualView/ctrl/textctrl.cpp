#include "widgets.h"
#include "frontend/win/ctrls/textEditor.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueTextCtrl, IValueWindow)

//****************************************************************************

#include "form.h"
#include "backend/metaData.h"
#include "backend/objCtor.h"

bool CValueTextCtrl::GetChoiceForm(CPropertyList* property)
{	
	property->AppendItem(wxT("default"), _("default"), wxNOT_FOUND);
	
	const IMetaData* metaData = GetMetaData();
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
	wxControlEditorCtrl* textEditor = new wxControlEditorCtrl(wxparent, wxID_ANY,
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
	wxControlEditorCtrl* textEditor = dynamic_cast<wxControlEditorCtrl*>(wxobject);

	if (textEditor != nullptr) {
		wxString textCaption = wxEmptyString;

		if (!m_propertySource->IsEmptyProperty()) {
			const IMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
			if (metaObject != nullptr) textCaption = metaObject->GetSynonym() + wxT(":");
		}

		textEditor->SetLabel(m_propertyCaption->IsEmptyProperty() ?
			textCaption : m_propertyCaption->GetValueAsString());

		if (!m_propertySource->IsEmptyProperty()) {
			ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
			if (srcObject != nullptr) {
				srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
			}
		}

		if (!appData->DesignerMode()) {
			textEditor->SetValue(m_selValue.GetString());
		}

		textEditor->SetPasswordMode(m_propertyPasswordMode->GetValueAsBoolean());
		textEditor->SetMultilineMode(m_propertyMultilineMode->GetValueAsBoolean());
		textEditor->SetTextEditMode(m_propertyTexteditMode->GetValueAsBoolean());

		if (!appData->DesignerMode()) {
			
			textEditor->ShowSelectButton(m_propertySelectButton->GetValueAsBoolean());
			textEditor->ShowOpenButton(m_propertyOpenButton->GetValueAsBoolean());
			textEditor->ShowClearButton(m_propertyClearButton->GetValueAsBoolean());

			textEditor->Bind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTextCtrl::OnSelectButtonPressed, this);
			textEditor->Bind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTextCtrl::OnOpenButtonPressed, this);
			textEditor->Bind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTextCtrl::OnClearButtonPressed, this);

			textEditor->Bind(wxEVT_CONTROL_TEXT_ENTER, &CValueTextCtrl::OnTextEnter, this);
			textEditor->Bind(wxEVT_CONTROL_TEXT_INPUT, &CValueTextCtrl::OnTextUpdated, this);
			textEditor->Bind(wxEVT_CONTROL_TEXT_CLEAR, &CValueTextCtrl::OnTextUpdated, this);
		
			textEditor->Bind(wxEVT_KILL_FOCUS, &CValueTextCtrl::OnKillFocus, this);
		}
		else {
			textEditor->ShowSelectButton(m_propertySelectButton->GetValueAsBoolean());
			textEditor->ShowOpenButton(m_propertyOpenButton->GetValueAsBoolean());
			textEditor->ShowClearButton(m_propertyClearButton->GetValueAsBoolean());
		}
	}

	UpdateWindow(textEditor);
}

void CValueTextCtrl::Cleanup(wxObject* wxobject, IVisualHost* visualHost)
{
	wxControlEditorCtrl* textEditor = dynamic_cast<wxControlEditorCtrl*>(wxobject);

	if (textEditor != nullptr) {
		if (!appData->DesignerMode()) {
			
			textEditor->Unbind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTextCtrl::OnSelectButtonPressed, this);
			textEditor->Unbind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTextCtrl::OnOpenButtonPressed, this);
			textEditor->Unbind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTextCtrl::OnClearButtonPressed, this);

			textEditor->Unbind(wxEVT_CONTROL_TEXT_ENTER, &CValueTextCtrl::OnTextEnter, this);
			textEditor->Unbind(wxEVT_CONTROL_TEXT_INPUT, &CValueTextCtrl::OnTextUpdated, this);
			textEditor->Unbind(wxEVT_CONTROL_TEXT_CLEAR, &CValueTextCtrl::OnTextUpdated, this);
		
			textEditor->Unbind(wxEVT_KILL_FOCUS, &CValueTextCtrl::OnKillFocus, this);
		
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

	wxControlEditorCtrl* textEditor = dynamic_cast<wxControlEditorCtrl*>(GetWxObject());
	if (textEditor != nullptr) {
		textEditor->SetValue(m_selValue.GetString());
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