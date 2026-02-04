#include "widgets.h"
#include "frontend/win/ctrls/controlCheckboxEditor.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueCheckbox, IValueWindow)

//****************************************************************************

#include "form.h"
#include "backend/metaData.h"

ISourceObject* CValueCheckbox::GetSourceObject() const
{
	return m_formOwner != nullptr ?
		m_formOwner->GetSourceObject() : nullptr;
}

//****************************************************************************
//*                              Checkbox                                    *
//****************************************************************************

enum prop {
	eControlValue,
};

CValueCheckbox::CValueCheckbox() : IValueWindow(), ITypeControlFactory()//(eValueTypes::TYPE_BOOLEAN)
{
}

IMetaData* CValueCheckbox::GetMetaData() const
{
	return m_formOwner != nullptr ?
		m_formOwner->GetMetaData() : nullptr;
}

void CValueCheckbox::PrepareNames() const
{
	IValueFrame::PrepareNames();

	m_methodHelper->AppendProp(wxT("value"), eControlValue, eControl);
}

bool CValueCheckbox::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum); bool refreshColumn = false;
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eControlValue) {
			SetControlValue(varPropVal);
		}
	}

	return IValueFrame::SetPropVal(lPropNum, varPropVal);
}

bool CValueCheckbox::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eControlValue) {
			return GetControlValue(pvarPropVal);
		}
	}
	return IValueFrame::GetPropVal(lPropNum, pvarPropVal);
}

wxString CValueCheckbox::GetControlTitle() const
{
	if (!m_propertyTitle->IsEmptyProperty()) {
		return m_propertyTitle->GetValueAsTranslateString();
	}
	else if (!m_propertySource->IsEmptyProperty()) {
		const IValueMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
		wxASSERT(metaObject);
		return metaObject->GetSynonym();
	}
	return wxEmptyString;
}

wxObject* CValueCheckbox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxControlCheckbox* checkbox = new wxControlNavigationCheckbox(wxparent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	checkbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CValueCheckbox::OnClickedCheckbox, this);
	return checkbox;
}

void CValueCheckbox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
}

#include "backend/appData.h"

void CValueCheckbox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxControlCheckbox* checkbox = dynamic_cast<wxControlCheckbox*>(wxobject);

	if (checkbox != nullptr) {

		if (!m_propertySource->IsEmptyProperty()) {
			ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
			if (srcObject != nullptr) {
				srcObject->GetValueByMetaID(m_propertySource->GetValueAsSource(), m_selValue);
			}
		}

		checkbox->SetLabel(GetControlTitle());
		if (!appData->DesignerMode()) {
			checkbox->SetValue(m_selValue.GetBoolean());
		}
		checkbox->SetWindowStyle(
			m_propertyTitleLocation->GetValueAsInteger() == 1 ? wxALIGN_LEFT :
			wxALIGN_RIGHT
		);
	}

	UpdateWindow(checkbox);
}

void CValueCheckbox::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	wxControlCheckbox* checkbox = dynamic_cast<wxControlCheckbox*>(obj);
	if (checkbox != nullptr) {
		checkbox->Unbind(wxEVT_COMMAND_CHECKBOX_CLICKED, &CValueCheckbox::OnClickedCheckbox, this);
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

	pvarControlVal = ITypeControlFactory::AdjustValue(m_selValue);
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

	wxControlCheckbox* checkboxCtrl = dynamic_cast<wxControlCheckbox*>(GetWxObject());
	if (checkboxCtrl != nullptr) {
		checkboxCtrl->SetValue(varControlVal.GetBoolean());
	}

	return true;
}

//*******************************************************************
//*							 Data	                                *
//*******************************************************************

bool CValueCheckbox::LoadData(CMemoryReader& reader)
{
	wxString title; reader.r_stringZ(title);
	m_propertyTitle->SetValue(title);
	m_propertyTitleLocation->SetValue(reader.r_s32());
	if (!m_propertySource->LoadData(reader))
		return false;

	//events
	m_onCheckboxClicked->LoadData(reader);
	return IValueWindow::LoadData(reader);
}

bool CValueCheckbox::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(m_propertyTitle->GetValueAsString());
	writer.w_s32(m_propertyTitleLocation->GetValueAsInteger());
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