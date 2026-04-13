#include "metaAttributeObjectEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumItemMode, ibValue);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumSelectMode, ibValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(ibValueEnumItemMode, "ItemMode", string_to_clsid("EN_ITMO"));
ENUM_TYPE_REGISTER(ibValueEnumSelectMode, "SelectMode", string_to_clsid("EN_SEMO"));