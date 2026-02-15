////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value picture 
////////////////////////////////////////////////////////////////////////////

#include "valuePicture.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CValuePicture, CValue);

CValuePicture::CValuePicture(const CPictureDescription& pictureDesc) :
	CValue(eValueTypes::TYPE_VALUE), m_pictureDesc(pictureDesc)
{
}

bool CValuePicture::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	// Get from FS
	return CBackendPicture::LoadFromFile(
		paParams[0]->GetString(), m_pictureDesc);
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValuePicture, "StoragePicture", string_to_clsid("SY_PICTR"));
