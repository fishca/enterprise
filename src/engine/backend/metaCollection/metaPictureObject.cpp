#include "metaPictureObject.h"

//***********************************************************************
//*                            IntrfaceObject                           *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectPicture, IValueMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CValueMetaObjectPicture::CValueMetaObjectPicture(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObject(name, synonym, comment)
{
}

bool CValueMetaObjectPicture::LoadData(CMemoryReader& reader)
{
	return m_propertyPicture->LoadData(reader);
}

bool CValueMetaObjectPicture::SaveData(CMemoryWriter& writer)
{
	return m_propertyPicture->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectPicture, "picture", g_metaPictureCLSID);