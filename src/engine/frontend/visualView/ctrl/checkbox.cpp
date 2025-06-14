#include "widgets.h"
#include "frontend/win/ctrls/checkBoxEditor.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueCheckbox, IValueWindow)

//****************************************************************************

#include "form.h"
#include "backend/metaData.h"

ISourceObject* CValueCheckbox::GetSourceObject() const
{
	return m_formOwner ? m_formOwner->GetSourceObject()
		: nullptr;
}

//****************************************************************************
//*                              Checkbox                                    *
//****************************************************************************

CValueCheckbox::CValueCheckbox() : IValueWindow(), ITypeControlFactory()//(eValueTypes::TYPE_BOOLEAN)
{
}

IMetaData* CValueCheckbox::GetMetaData() const
{
	return m_formOwner ?
		m_formOwner->GetMetaData() : nullptr;
}

wxObject* CValueCheckbox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	CCheckBox* checkbox = new CCheckBox(wxparent, wxID_ANY,
		m_propertyCaption->GetValueAsString(),
		wxDefaultPosition,
		wxDefaultSize);

	checkbox->BindCheckBoxCtrl(&CValueCheckbox::OnClickedCheckbox, this);

	return checkbox;
}

void CValueCheckbox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
}

void CValueCheckbox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	CCheckBox* checkbox = dynamic_cast<CCheckBox*>(wxobject);

	if (checkbox != nullptr) {
		wxString textCaption = wxEmptyString;

		if (!m_propertySource->IsEmptyProperty()) {
			const ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
			if (srcObject != nullptr) {
				const IMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
				if (metaObject != nullptr)  textCaption = metaObject->GetSynonym() + wxT(":");	
				srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
			}
		}

		checkbox->SetCheckBoxLabel(m_propertyCaption->IsEmptyProperty() ?
			textCaption : m_propertyCaption->GetValueAsString());
		checkbox->SetCheckBoxValue(m_selValue.GetBoolean());
		checkbox->SetWindowStyle(
			m_propertyTitle->GetValueAsInteger() == 1 ? wxALIGN_LEFT :
			wxALIGN_RIGHT
		);

		checkbox->BindCheckBoxCtrl(&CValueCheckbox::OnClickedCheckbox, this);
	}

	UpdateWindow(checkbox);
	UpdateLabelSize(checkbox);
}

void CValueCheckbox::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	CCheckBox* checkbox = dynamic_cast<CCheckBox*>(obj);
	if (checkbox != nullptr) {
		checkbox->UnbindCheckBoxCtrl(&CValueCheckbox::OnClickedCheckbox, this);
	}
}

//*******************************************************************
//*							 Control value	                        *
//*******************************************************************

bool CValueCheckbox::GetControlValue(CValue& pvarControlVal) const
{
	if (!m_propertySource->IsEmptyProperty() && m_formOwner->GetSourceObject()) {
		ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
		if (srcObject != nullptr)
			return srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), pvarControlVal);
	}

	pvarControlVal = m_selValue;
	return true;
}

#include "backend/system/value/valueType.h"

bool CValueCheckbox::SetControlValue(const CValue& varControlVal)
{
	if (!m_propertySource->IsEmptyProperty() && m_formOwner->GetSourceObject()) {
		ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
		if (srcObject != nullptr)
			srcObject->SetValueByMetaID(m_propertySource->GetValueAsSource(), varControlVal);
	}

	m_selValue = varControlVal.GetBoolean();

	CCheckBox* checkboxCtrl = dynamic_cast<CCheckBox*>(GetWxObject());
	if (checkboxCtrl != nullptr) {
		checkboxCtrl->SetCheckBoxValue(varControlVal.GetBoolean());
	}

	return true;
}

//*******************************************************************
//*							 Data	                                *
//*******************************************************************

bool CValueCheckbox::LoadData(CMemoryReader& reader)
{
	wxString caption; reader.r_stringZ(caption);
	m_propertyCaption->SetValue(caption);
	m_propertyTitle->SetValue(reader.r_s32());
	if (!m_propertySource->LoadData(reader))
		return false;
	
	//events
	m_onCheckboxClicked->LoadData(reader);	
	return IValueWindow::LoadData(reader);
}

bool CValueCheckbox::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(m_propertyCaption->GetValueAsString());
	writer.w_s32(m_propertyTitle->GetValueAsInteger());
	if (!m_propertySource->SaveData(writer))
		return false;

	//events
	m_onCheckboxClicked->SaveData(writer);
	return IValueWindow::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

CONTROL_TYPE_REGISTER(CValueCheckbox, "checkbox", "widget", string_to_clsid("CT_CHKB"));