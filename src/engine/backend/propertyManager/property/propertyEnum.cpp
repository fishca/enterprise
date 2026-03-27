#include "propertyEnum.h"

//load & save object in control 
bool ibPropertyEnumBase::LoadData(ibReaderMemory& reader)
{
	const s32& enumVariant = reader.r_s32();
	ibPropertyEnumBase::SetValue(enumVariant);
	return true;
}

bool ibPropertyEnumBase::SaveData(ibWriterMemory& writer)
{
	const s32& enumVariant = ibPropertyEnumBase::GetValueAsInteger();
	writer.w_s32(enumVariant);
	return true;
}