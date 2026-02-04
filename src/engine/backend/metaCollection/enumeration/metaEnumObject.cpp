////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-enumerations
////////////////////////////////////////////////////////////////////////////

#include "metaEnumObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectEnum, IValueMetaObject)

bool CValueMetaObjectEnum::LoadData(CMemoryReader &reader)
{
	return true;
}

bool CValueMetaObjectEnum::SaveData(CMemoryWriter &writer)
{
	return true;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectEnum, "enum", g_metaEnumCLSID);
