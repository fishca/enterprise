#include "metaAttributeObjectEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumItemMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumSelectMode, CValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(CValueEnumItemMode, "ItemMode", string_to_clsid("EN_ITMO"));
ENUM_TYPE_REGISTER(CValueEnumSelectMode, "SelectMode", string_to_clsid("EN_SEMO"));