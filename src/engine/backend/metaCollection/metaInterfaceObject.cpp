#include "metaInterfaceObject.h"

//***********************************************************************
//*                            IntrfaceObject                           *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectInterface, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CMetaObjectInterface::CMetaObjectInterface(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

bool CMetaObjectInterface::LoadData(CMemoryReader& reader)
{
	return m_propertyPicture->LoadData(reader);
}

bool CMetaObjectInterface::SaveData(CMemoryWriter& writer)
{
	return m_propertyPicture->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectInterface, "interface", g_metaInterfaceCLSID);