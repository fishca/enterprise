#include "informationRegisterEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumPeriodicity, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumWriteRegisterMode, CValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(CValueEnumPeriodicity, "InformationPeriodicity", string_to_clsid("EN_PRST"));
ENUM_TYPE_REGISTER(CValueEnumWriteRegisterMode, "InformationWriteRegisterMode", string_to_clsid("EN_WMOD"));