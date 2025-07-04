#include "accumulationRegister.h"

void CMetaObjectAccumulationRegister::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (GetRegisterType() == eRegisterType::eBalances) {
		(*m_propertyAttributeRecordType)->ClearFlag(metaDisableFlag);
	}
	else if (GetRegisterType() == eRegisterType::eTurnovers) {
		(*m_propertyAttributeRecordType)->SetFlag(metaDisableFlag);
	}

	IMetaObjectRegisterData::OnPropertyChanged(property, oldValue, newValue);
}