#include "propertyList.h"

//get property for grid  	
wxObject* (*ibPropertyList::ms_propertyList)(const wxString&, const wxString&, const wxPGChoices&, const int&) = nullptr;

//base property for "list"
bool ibPropertyList::SetDataValue(const ibValue& varPropVal)
{
	if (!m_functor->Invoke(this))
		return false;

	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		const ibValue* selValue = m_listPropValue.GetItemValue(idx);
		if ((selValue != nullptr && *selValue == varPropVal) || (selValue == nullptr && varPropVal == wxEmptyValue)) {
			SetValue(stringUtils::IntToStr(m_listPropValue.GetItemId(idx)));
			return true;
		}
	}
	return false;
};

bool ibPropertyList::GetDataValue(ibValue& pvarPropVal) const
{
	if (!m_functor->Invoke(const_cast<ibPropertyList*>(this)))
		return false;

	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		if (m_listPropValue.GetItemId(idx) == GetValueAsInteger()) {
			pvarPropVal = m_listPropValue.GetItemValue(idx);
			return true;
		}
	}
	return false;
};

bool ibPropertyList::LoadData(ibReaderMemory& reader)
{
	SetValue((long)reader.r_s32());
	return true;
};

bool ibPropertyList::SaveData(ibWriterMemory& writer)
{
	writer.w_s32(GetValueAsInteger());
	return true;
};