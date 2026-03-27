#include "informationRegisterEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumPeriodicity, ibValue);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumWriteRegisterMode, ibValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(ibValueEnumPeriodicity, "InformationPeriodicity", string_to_clsid("EN_PRST"));
ENUM_TYPE_REGISTER(ibValueEnumWriteRegisterMode, "InformationWriteRegisterMode", string_to_clsid("EN_WMOD"));