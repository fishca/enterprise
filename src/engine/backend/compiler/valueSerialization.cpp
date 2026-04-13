#include "value.h"

bool CValue::DoSerialize(wxString& strValue) const
{
	if (m_typeClass == eValueTypes::TYPE_REFFER) 
		return m_pRef->DoSerialize(strValue);
	
	switch (m_typeClass) {
	case eValueTypes::TYPE_NULL:
		break;
	case eValueTypes::TYPE_BOOLEAN:

		break;
	case eValueTypes::TYPE_NUMBER:
		break;
	case eValueTypes::TYPE_STRING:
		break;
	case eValueTypes::TYPE_DATE:
		break;
	default:
		break;
	}

	return false;
}

bool CValue::DoDeserialize(const wxString& strValue)
{
	if (m_typeClass == eValueTypes::TYPE_REFFER)
		return m_pRef->DoDeserialize(strValue);

	switch (m_typeClass) {
	case eValueTypes::TYPE_NULL:
		break;
	case eValueTypes::TYPE_BOOLEAN:
		break;
	case eValueTypes::TYPE_NUMBER:
		break;
	case eValueTypes::TYPE_STRING:
		break;
	case eValueTypes::TYPE_DATE:
		break;
	default:
		break;
	}

	return false;
}
