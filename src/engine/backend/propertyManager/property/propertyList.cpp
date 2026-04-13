#include "propertyList.h"

//base property for "list"
bool CPropertyList::SetDataValue(const CValue& varPropVal) 
{
	if (!m_functor->Invoke(this))
		return false;

	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		const CValue *selValue = m_listPropValue.GetItemValue(idx); 
		if ((selValue != nullptr && *selValue == varPropVal) || (selValue == nullptr && varPropVal == wxEmptyValue)) {
			SetValue(stringUtils::IntToStr(m_listPropValue.GetItemId(idx)));
			return true;
		}
	}
	return false;
};

bool CPropertyList::GetDataValue(CValue& pvarPropVal) const 
{
	if (!m_functor->Invoke(const_cast<CPropertyList*>(this)))
		return false;

	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		if (m_listPropValue.GetItemId(idx) == GetValueAsInteger()) {
			pvarPropVal = m_listPropValue.GetItemValue(idx);
			return true;
		}
	}
	return false;
};

bool CPropertyList::LoadData(CMemoryReader& reader)
{
	SetValue((long)reader.r_s32());
	return true;
};

bool CPropertyList::SaveData(CMemoryWriter& writer) 
{
	writer.w_s32(GetValueAsInteger());
	return true;
};