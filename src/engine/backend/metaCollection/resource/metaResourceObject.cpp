#include "metaResourceObject.h"
#include "backend/metadata.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectResource, CValueMetaObjectAttribute);

eSelectorDataType CValueMetaObjectResource::GetFilterDataType() const
{
	IValueMetaObjectGenericData* metaObject = dynamic_cast<IValueMetaObjectGenericData*>(m_parent);
	if (metaObject->GetClassType() == g_metaInformationRegisterCLSID) 
		return metaObject->GetFilterDataType();
	return eSelectorDataType::eSelectorDataType_resource;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectResource, "resource", g_metaResourceCLSID);