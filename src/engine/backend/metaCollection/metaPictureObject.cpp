#include "metaPictureObject.h"

//***********************************************************************
//*                            IntrfaceObject                           *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectPicture, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CMetaObjectPicture::CMetaObjectPicture(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

bool CMetaObjectPicture::LoadData(CMemoryReader& reader)
{
	return m_propertyPicture->LoadData(reader);
}

bool CMetaObjectPicture::SaveData(CMemoryWriter& writer)
{
	return m_propertyPicture->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectPicture, "picture", g_metaPictureCLSID);