////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : form - controls 
////////////////////////////////////////////////////////////////////////////

#include "form.h"


//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CValueForm::CValueFormCollectionControl, CValue);
//////////////////////////////////////////////////////////////////////

CValueForm::CValueFormCollectionControl::CValueFormCollectionControl() : CValue(eValueTypes::TYPE_VALUE, true),
m_formOwner(nullptr), m_methodHelper(nullptr)
{
}

CValueForm::CValueFormCollectionControl::CValueFormCollectionControl(CValueForm* ownerFrame) : CValue(eValueTypes::TYPE_VALUE, true),
m_formOwner(ownerFrame), m_methodHelper(new CMethodHelper())
{
}

#include "backend/system/value/valueMap.h"

CValueForm::CValueFormCollectionControl::~CValueFormCollectionControl()
{
	wxDELETE(m_methodHelper);
}

CValue CValueForm::CValueFormCollectionControl::GetIteratorEmpty()
{
	return CValue::CreateAndPrepareValueRef<CValueContainer::CValueReturnContainer>();
}

CValue CValueForm::CValueFormCollectionControl::GetIteratorAt(unsigned int idx)
{
	if (m_formOwner->m_listControl.size() < idx)
		return CValue();

	auto structurePos = m_formOwner->m_listControl.begin();
	std::advance(structurePos, idx);

	return CValue::CreateAndPrepareValueRef<CValueContainer::CValueReturnContainer>(
		(*structurePos)->GetControlName(),
		CValue(*structurePos)
	);
}

bool CValueForm::CValueFormCollectionControl::GetAt(const CValue& varKeyValue, CValue& pvarValue)
{
	const number_t& number = varKeyValue.GetNumber();
	if (m_formOwner->m_listControl.size() < number.ToUInt())
		return false;
	
	auto it = m_formOwner->m_listControl.begin();
	std::advance(it, number.ToUInt());
	pvarValue = *it;

	return true;
}

bool CValueForm::CValueFormCollectionControl::Property(const CValue& varKeyValue, CValue& cValueFound)
{
	const wxString& key = varKeyValue.GetString();
	auto it = std::find_if(m_formOwner->m_listControl.begin(), m_formOwner->m_listControl.end(),
		[key](IValueControl* control) {
			return stringUtils::CompareString(key, control->GetControlName());
		}
	);
	
	if (it != m_formOwner->m_listControl.end()) {
		cValueFound = *it;
		return true;
	}
	
	return false;
}

enum
{
	enControlCreate,
	enControlFind,
	enControlRemove,
	enControlProperty,
	enControlCount
};

void CValueForm::CValueFormCollectionControl::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendFunc("createControl", 2, "createControl(typeControl, parentElement)");
	m_methodHelper->AppendFunc("findControl", 1, "findControl(controlName)");
	m_methodHelper->AppendProc("removeControl", 1, "removeControl(controlElement)");
	m_methodHelper->AppendFunc("property", 2, "property(key, valueFound)");
	m_methodHelper->AppendFunc("count", "count()");

	for (auto control : m_formOwner->m_listControl) {
		m_methodHelper->AppendProp(
			control->GetControlName(),
			true,
			false,
			control->GetControlID()
		);
	}
}

bool CValueForm::CValueFormCollectionControl::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	wxASSERT(m_formOwner);
	pvarPropVal = m_formOwner->FindControlByID(
		m_methodHelper->GetPropData(lPropNum)
	);
	return !pvarPropVal.IsEmpty();
}

#include "backend/system/value/valueType.h"

bool CValueForm::CValueFormCollectionControl::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enControlRemove:
		m_formOwner->RemoveControl(paParams[0]->ConvertToType<IValueFrame>());
		return true; 
	}
	return false;
}

bool CValueForm::CValueFormCollectionControl::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enControlCreate:
		pvarRetValue = m_formOwner->CreateControl(paParams[0]->ConvertToType<CValueType>(), lSizeArray > 1 ? paParams[1]->ConvertToType<IValueFrame>() : CValue());
		return true;
	case enControlFind:
		pvarRetValue = m_formOwner->FindControl(paParams[0]->GetString());
		return true;
	case enControlProperty:
		pvarRetValue = Property(*paParams[0], lSizeArray > 1 ? *paParams[1] : CValue());
		return true;
	case enControlCount:
		pvarRetValue = Count();
		return true;
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

SYSTEM_TYPE_REGISTER(CValueForm::CValueFormCollectionControl, "formControl", string_to_clsid("VL_CNTR"));