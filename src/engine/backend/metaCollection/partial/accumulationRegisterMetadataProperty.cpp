#include "accumulationRegister.h"

void ibValueMetaObjectAccumulationRegister::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (GetRegisterType() == ibRegisterType::eBalances) {
		(*m_propertyAttributibRecordType)->ClearFlag(metaDisableFlag);
	}
	else if (GetRegisterType() == ibRegisterType::eTurnovers) {
		(*m_propertyAttributibRecordType)->SetFlag(metaDisableFlag);
	}

	ibValueMetaObjectRegisterData::OnPropertyChanged(property, oldValue, newValue);
}