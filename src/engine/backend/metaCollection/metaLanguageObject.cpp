#include "metaLanguageObject.h"

//***********************************************************************
//*                            MetaObjectLanguage                       *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectLanguage, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

#include "backend/metadata.h"

bool CMetaObjectLanguage::IsValidCode(const wxString& code)
{
	if (m_metaData == nullptr)
		return false;

	for (const auto language : m_metaData->GetAnyArrayObject<CMetaObjectLanguage>(g_metaLanguageCLSID)) {
		if (language != this && stringUtils::CompareString(code, language->GetLanguageCode()))
			return false;
	}

	return true;
}

CMetaObjectLanguage::CMetaObjectLanguage(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

bool CMetaObjectLanguage::LoadData(CMemoryReader& reader)
{
	return m_propertyCode->LoadData(reader);
}

bool CMetaObjectLanguage::SaveData(CMemoryWriter& writer)
{
	return m_propertyCode->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectLanguage, "language", g_metaLanguageCLSID);