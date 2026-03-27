#include "documentEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumDocumentWriteMode, ibValue);
wxIMPLEMENT_DYNAMIC_CLASS(ibValueEnumDocumentPostingMode, ibValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(ibValueEnumDocumentWriteMode, "DocumentWriteMode", string_to_clsid("EN_WRMO"));
ENUM_TYPE_REGISTER(ibValueEnumDocumentPostingMode, "DocumentPostingMode", string_to_clsid("EN_POMO"));