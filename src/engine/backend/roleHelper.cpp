#include "roleHelper.h"

void CRole::InitRole(IAccessObject* obj, const bool& value)
{
	m_owner->AddRole(this);
	m_defValue = value;
}

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
	if (!DoAccessRight(role))
		return false;
	return role->GetDefValue();
}

bool IAccessObject::SetRight(const CRole* role, const meta_identifier_t& id, const bool& val)
{
	if (role == nullptr)
		return false;
	m_valRoles[id].insert_or_assign(role->GetName(), val);
	DoSetRight(role);
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

#pragma endregion