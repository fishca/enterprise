#include "metaDimensionObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectDimension, ibValueMetaObjectAttribute)

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectDimension, "Dimension", g_metaDimensionCLSID);