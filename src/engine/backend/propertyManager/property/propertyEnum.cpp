#include "propertyEnum.h"

//load & save object in control 
bool IPropertyEnum::LoadData(CMemoryReader& reader)
{
	const s32& enumVariant = reader.r_s32();
	IPropertyEnum::SetValue(enumVariant);
	return true;
}

bool IPropertyEnum::SaveData(CMemoryWriter& writer)
{
	const s32& enumVariant = IPropertyEnum::GetValueAsInteger();
	writer.w_s32(enumVariant);
	return true;
}