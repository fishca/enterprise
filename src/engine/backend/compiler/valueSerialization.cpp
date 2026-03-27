#include "value.h"

bool ibValue::DoSerialize(wxString& strValue) const
{
	if (m_typeClass == ibValueTypes::TYPE_REFFER) 
		return m_pRef->DoSerialize(strValue);
	
	switch (m_typeClass) {
	case ibValueTypes::TYPE_NULL:
		break;
	case ibValueTypes::TYPE_BOOLEAN:

		break;
	case ibValueTypes::TYPE_NUMBER:
		break;
	case ibValueTypes::TYPE_STRING:
		break;
	case ibValueTypes::TYPE_DATE:
		break;
	default:
		break;
	}

	return false;
}

bool ibValue::DoDeserialize(const wxString& strValue)
{
	if (m_typeClass == ibValueTypes::TYPE_REFFER)
		return m_pRef->DoDeserialize(strValue);

	switch (m_typeClass) {
	case ibValueTypes::TYPE_NULL:
		break;
	case ibValueTypes::TYPE_BOOLEAN:
		break;
	case ibValueTypes::TYPE_NUMBER:
		break;
	case ibValueTypes::TYPE_STRING:
		break;
	case ibValueTypes::TYPE_DATE:
		break;
	default:
		break;
	}

	return false;
}
