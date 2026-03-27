////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : meta-enumerations
////////////////////////////////////////////////////////////////////////////

#include "metaEnumObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(ibValueMetaObjectEnum, ibValueMetaObject)

bool ibValueMetaObjectEnum::LoadData(ibReaderMemory &reader)
{
	return true;
}

bool ibValueMetaObjectEnum::SaveData(ibWriterMemory &writer)
{
	return true;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(ibValueMetaObjectEnum, "Enum", g_metaEnumCLSID);
