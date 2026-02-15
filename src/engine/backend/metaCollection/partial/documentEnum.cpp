#include "documentEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumDocumentWriteMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumDocumentPostingMode, CValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(CValueEnumDocumentWriteMode, "DocumentWriteMode", string_to_clsid("EN_WRMO"));
ENUM_TYPE_REGISTER(CValueEnumDocumentPostingMode, "DocumentPostingMode", string_to_clsid("EN_POMO"));