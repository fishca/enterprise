#include "metaLanguageObject.h"

//***********************************************************************
//*                            MetaObjectLanguage                       *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectLanguage, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CMetaObjectLanguage::CMetaObjectLanguage(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

bool CMetaObjectLanguage::LoadData(CMemoryReader& reader)
{
	return true;
}

bool CMetaObjectLanguage::SaveData(CMemoryWriter& writer)
{
	return true;
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectLanguage, "language", g_metaLanguageCLSID);