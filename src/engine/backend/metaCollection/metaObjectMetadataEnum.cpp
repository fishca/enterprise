#include "metaObjectMetadataEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumVersion, ibValue);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumSyntax, ibValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(ibValueEnumVersion, "ProgramVersion", string_to_clsid("EN_VRSN"));
ENUM_TYPE_REGISTER(ibValueEnumSyntax, "ProgramSyntax", string_to_clsid("EN_SYNTX"));