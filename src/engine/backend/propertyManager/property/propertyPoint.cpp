#include "propertyPoint.h"
#include "backend/system/value/valuePoint.h"

//base property for "point"
bool ibPropertyPoint::SetDataValue(const ibValue& varPropVal)
{
	ibValuePoint* valuePoint = varPropVal.ConvertToType<ibValuePoint>();
	if (valuePoint == nullptr)
		return false;
	SetValue(valuePoint->m_point);
	return true;
}

bool ibPropertyPoint::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValuePoint>(GetValueAsPoint());
	return true;
}

bool ibPropertyPoint::LoadData(ibReaderMemory& reader)
{
	ibPropertyPoint::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyPoint::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(ibPropertyPoint::GetValueAsString());
	return true;
}