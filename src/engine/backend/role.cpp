#include "role.h"
#include "metaCollection/metaObject.h"

void CRole::InitRole(IMetaObject* metaObject, const bool& value)
{
	m_owner->AddRole(this);
	m_defValue = value;
}
