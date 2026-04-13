#include "accumulationRegisterEnum.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumAccumulationRegisterType, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumAccumulationRegisterRecordType, CValue);

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

//add new enumeration
ENUM_TYPE_REGISTER(CValueEnumAccumulationRegisterType, "AccumulationRegisterType", string_to_clsid("EN_RTTP"));
ENUM_TYPE_REGISTER(CValueEnumAccumulationRegisterRecordType, "AccumulationRecordType", g_enumRecordTypeCLSID);