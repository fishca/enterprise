#include "roleHelper.h"

#define roleBlock 0x200000

////////////////////////////////////////////////////////////////////////

void CRole::InitRole(IAccessObject* obj, const bool& value)
{
	m_owner->AddRole(this);
	m_defValue = value;
}

////////////////////////////////////////////////////////////////////////

#pragma region role 

IAccessObject::~IAccessObject()
{
	for (auto& role : m_roles) { wxDELETE(role.second); }
	m_roles.clear();
}

void IAccessObject::AddRole(CRole* role)
{
	m_roles.emplace_back(role->GetName(), role);
}

bool IAccessObject::AccessRight(const CRole* role, const meta_identifier_t& id) const
{
	if (!DoAccessRight(role))
		return false;

	auto roleData = m_valRoles.find(id);
	if (roleData != m_valRoles.end()) {
		auto foundedData = std::find_if(roleData->second.begin(), roleData->second.end(), [role](const std::pair<wxString, bool >& pair)
			{
				return stringUtils::CompareString(role->GetName(), pair.first);
			}
		);
		if (foundedData != roleData->second.end())
			return foundedData->second;
	}

	return role->GetDefValue();
}

bool IAccessObject::SetRight(const CRole* role, const meta_identifier_t& id, const bool& set)
{
	if (role == nullptr)
		return false;
	m_valRoles[id].insert_or_assign(role->GetName(), set);
	DoSetRight(role, set);
	return true;
}

CRole* IAccessObject::GetRole(const wxString& nameParam) const
{
	auto it = std::find_if(m_roles.begin(), m_roles.end(),
		[nameParam](const std::pair<wxString, CRole*>& pair)
		{
			return stringUtils::CompareString(nameParam, pair.first);
		}
	);

	if (it != m_roles.end())
		return &(*it->second);

	return nullptr;
}

CRole* IAccessObject::GetRole(unsigned int idx) const
{
	assert(idx < m_roles.size());

	auto it = m_roles.begin();
	unsigned int i = 0;
	while (i < idx && it != m_roles.end()) {
		i++;
		it++;
	}

	if (it != m_roles.end())
		return &(*it->second);
	return nullptr;
}

bool IAccessObject::LoadRole(CMemoryReader& dataReader)
{
	wxMemoryBuffer buf;
	if (dataReader.r_chunk(roleBlock, buf)) {
		std::shared_ptr<CMemoryReader> dataRoleReader(new CMemoryReader(buf));
		unsigned int countRole = dataRoleReader->r_u32(); m_valRoles.clear();
		for (unsigned int idx = 0; idx < countRole; idx++) {
			unsigned int countData = dataRoleReader->r_u32();
			meta_identifier_t id = dataRoleReader->r_s32();
			for (unsigned int idc = 0; idc < countData; idc++) {
				wxString roleName = dataRoleReader->r_stringZ();
				bool roleValue = dataRoleReader->r_u8();
				m_valRoles[id].insert_or_assign(roleName, roleValue);
			}
		}

		return true;
	}

	return false;
}

bool IAccessObject::SaveRole(CMemoryWriter& dataWritter)
{
	CMemoryWriter dataRoleWritter;
	dataRoleWritter.w_u32(m_valRoles.size());
	for (auto role_id : m_valRoles) {
		dataRoleWritter.w_u32(role_id.second.size());
		dataRoleWritter.w_s32(role_id.first); // role id
		for (auto role_data : role_id.second) {
			dataRoleWritter.w_stringZ(role_data.first); //strName role
			dataRoleWritter.w_u8(role_data.second); // value true/false
		}
	}

	dataWritter.w_chunk(roleBlock, dataRoleWritter.buffer());
	return true;
}

#pragma endregion