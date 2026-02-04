#include "metaDimensionObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectDimension, CValueMetaObjectAttribute)

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectDimension, "dimension", g_metaDimensionCLSID);