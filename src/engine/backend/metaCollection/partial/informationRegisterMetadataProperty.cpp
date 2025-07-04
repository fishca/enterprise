#include "informationRegister.h"

void CMetaObjectInformationRegister::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		(*m_propertyAttributeLineActive)->ClearFlag(metaDisableFlag);
		(*m_propertyAttributeRecorder)->ClearFlag(metaDisableFlag);
		(*m_propertyAttributeLineNumber)->ClearFlag(metaDisableFlag);
	}
	else if (GetWriteRegisterMode() == eWriteRegisterMode::eIndependent) {
		(*m_propertyAttributeLineActive)->SetFlag(metaDisableFlag);
		(*m_propertyAttributeRecorder)->SetFlag(metaDisableFlag);
		(*m_propertyAttributeLineNumber)->SetFlag(metaDisableFlag);
	}

	if (GetPeriodicity() != ePeriodicity::eNonPeriodic ||
		GetWriteRegisterMode() == eWriteRegisterMode::eSubordinateRecorder) {
		(*m_propertyAttributePeriod)->ClearFlag(metaDisableFlag);
	}
	else {
		(*m_propertyAttributePeriod)->SetFlag(metaDisableFlag);
	}

	IMetaObjectRegisterData::OnPropertyChanged(property, oldValue, newValue);
}