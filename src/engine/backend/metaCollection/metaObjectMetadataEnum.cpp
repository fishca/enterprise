#include "metaObjectMetadataEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumVersion, ibValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(ibValueEnumVersion, "ProgramVersion", string_to_clsid("EN_VRSN"));