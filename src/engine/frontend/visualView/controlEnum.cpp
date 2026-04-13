////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value enum  
////////////////////////////////////////////////////////////////////////////

#include "controlEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumOrient, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumStretch, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumOrientNotebookPage, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumHorizontalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumVerticalAlignment, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumTitleLocation, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumRepresentation, CValue);

//add new enumeration
ENUM_TYPE_REGISTER(CValueEnumOrient, "WindowOrient", string_to_clsid("EN_WORI"));
ENUM_TYPE_REGISTER(CValueEnumStretch, "WindowStretch", string_to_clsid("EN_STRH"));
ENUM_TYPE_REGISTER(CValueEnumOrientNotebookPage, "WindowOrientNotebookPage", string_to_clsid("EN_WNKP"));
ENUM_TYPE_REGISTER(CValueEnumHorizontalAlignment, "WindowHorizontalAlignment", string_to_clsid("EN_WHGT"));
ENUM_TYPE_REGISTER(CValueEnumVerticalAlignment, "WindowVerticalAlignment", string_to_clsid("EN_WAGT"));
ENUM_TYPE_REGISTER(CValueEnumTitleLocation, "TitleLocation", string_to_clsid("EN_TILC"));
ENUM_TYPE_REGISTER(CValueEnumRepresentation, "Representation", string_to_clsid("EN_RPRT"));