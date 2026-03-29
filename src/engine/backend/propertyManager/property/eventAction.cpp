#include "eventAction.h"
#include "backend/propertyManager/property/variant/variantAction.h"

wxVariantData* ibEventAction::CreateVariantData(const ibPropertyObject* property, const ibActionDescription& act) const
{
	return new ibVariantDataAction(act);
}

////////////////////////////////////////////////////////////////////////

ibActionID ibEventAction::GetValueAsInteger() const {
	const ibActionID sel = GetValueAsActionDesc().GetSystemAction();
	if (m_functor->Invoke(const_cast<ibEventAction*>(this))) {
		for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
			if (m_listPropValue.GetItemId(idx) == sel) {
				return sel;
			}
		}
	}
	return wxNOT_FOUND;
}

wxString ibEventAction::GetValueAsString() const
{
	return GetValueAsActionDesc().GetCustomAction();
}

ibActionDescription& ibEventAction::GetValueAsActionDesc() const
{
	return get_cell_variant<ibVariantDataAction>()->GetValueAsActionDesc();
}

void ibEventAction::SetValue(const ibActionDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

//base property for "action"
bool ibEventAction::SetDataValue(const ibValue& varPropVal)
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

bool ibEventAction::GetDataValue(ibValue& pvarPropVal) const
{
	if (!m_functor->Invoke(const_cast<ibEventAction*>(this)))
		return false;
	for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
		if (m_listPropValue.GetItemId(idx) == GetValueAsInteger()) {
			pvarPropVal = m_listPropValue.GetItemValue(idx);
			return true;
		}
	}
	return false;
}

bool ibActionDescriptionMemory::LoadData(ibReaderMemory& reader, ibActionDescription& typeDesc)
{
	typeDesc.m_lAction = reader.r_s32();
	typeDesc.m_strAction = reader.r_stringZ();
	return true;
}

bool ibActionDescriptionMemory::SaveData(ibWriterMemory& writer, ibActionDescription& typeDesc)
{
	writer.w_s32(typeDesc.m_lAction);
	writer.w_stringZ(typeDesc.m_strAction);
	return true;
}


bool ibEventAction::LoadData(ibReaderMemory& reader)
{
	return ibActionDescriptionMemory::LoadData(reader, GetValueAsActionDesc());
}

bool ibEventAction::SaveData(ibWriterMemory& writer)
{
	return ibActionDescriptionMemory::SaveData(writer, GetValueAsActionDesc());
}