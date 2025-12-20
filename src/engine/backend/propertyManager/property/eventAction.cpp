#include "eventAction.h"
#include "backend/propertyManager/property/variant/variantAction.h"

wxVariantData* CEventAction::CreateVariantData(const IPropertyObject* property, const CActionDescription& act) const
{
	return new wxVariantDataAction(act);
}

////////////////////////////////////////////////////////////////////////

action_identifier_t CEventAction::GetValueAsInteger() const {
	const action_identifier_t sel = GetValueAsActionDesc().GetSystemAction();
	if (m_functor->Invoke(const_cast<CEventAction*>(this))) {
		for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
			if (m_listPropValue.GetItemId(idx) == sel) {
				return sel;
			}
		}
	}
	return wxNOT_FOUND;
}

wxString CEventAction::GetValueAsString() const
{
	return GetValueAsActionDesc().GetCustomAction();
}

CActionDescription& CEventAction::GetValueAsActionDesc() const
{
	return get_cell_variant<wxVariantDataAction>()->GetValueAsActionDesc();
}

void CEventAction::SetValue(const CActionDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

//base property for "action"
bool CEventAction::SetDataValue(const CValue& varPropVal)
{
	if (!m_functor->Invoke(this))
		return false;
	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		if (m_listPropValue.GetItemValue(idx) == varPropVal) {
			SetValue(m_listPropValue.GetItemId(idx));
			return true;
		}
	}
	return false;
}

bool CEventAction::GetDataValue(CValue& pvarPropVal) const
{
	if (!m_functor->Invoke(const_cast<CEventAction*>(this)))
		return false;
	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		if (m_listPropValue.GetItemId(idx) == GetValueAsInteger()) {
			pvarPropVal = m_listPropValue.GetItemValue(idx);
			return true;
		}
	}
	return false;
}

bool CActionDescriptionMemory::LoadData(CMemoryReader& reader, CActionDescription& typeDesc)
{
	typeDesc.m_lAction = reader.r_s32();
	typeDesc.m_strAction = reader.r_stringZ();
	return true;
}

bool CActionDescriptionMemory::SaveData(CMemoryWriter& writer, CActionDescription& typeDesc)
{
	writer.w_s32(typeDesc.m_lAction);
	writer.w_stringZ(typeDesc.m_strAction);
	return true;
}


bool CEventAction::LoadData(CMemoryReader& reader)
{
	return CActionDescriptionMemory::LoadData(reader, GetValueAsActionDesc());
}

bool CEventAction::SaveData(CMemoryWriter& writer)
{
	return CActionDescriptionMemory::SaveData(writer, GetValueAsActionDesc());
}