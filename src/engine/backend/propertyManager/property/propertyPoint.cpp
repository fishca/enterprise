#include "propertyPoint.h"
#include "backend/system/value/valuePoint.h"

//base property for "point"
bool CPropertyPoint::SetDataValue(const CValue& varPropVal)
{
	CValuePoint* valuePoint = varPropVal.ConvertToType<CValuePoint>();
	if (valuePoint == nullptr)
		return false;
	SetValue(valuePoint->m_point);
	return true;
}

bool CPropertyPoint::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValuePoint>(GetValueAsPoint());
	return true;
}

bool CPropertyPoint::LoadData(CMemoryReader& reader)
{
	CPropertyPoint::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyPoint::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(CPropertyPoint::GetValueAsString());
	return true;
}