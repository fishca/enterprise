#include "metaLanguageObject.h"

//***********************************************************************
//*                            MetaObjectLanguage                       *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectLanguage, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

#include "backend/metadata.h"

bool CMetaObjectLanguage::IsValidCode(const wxString& strLangCode)
{
	if (m_metaData == nullptr)
		return false;

	for (const auto language : m_metaData->GetAnyArrayObject<CMetaObjectLanguage>(g_metaLanguageCLSID)) {
		if (language != this && stringUtils::CompareString(strLangCode, language->GetCode()))
			return false;
	}

	return true;
}

CMetaObjectLanguage::CMetaObjectLanguage(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

bool CMetaObjectLanguage::OnDeleteMetaObject()
{
	const IMetaObject* commonObject = m_metaData->GetCommonMetaObject();

	return true;
}

bool CMetaObjectLanguage::OnBeforeRunMetaObject(int flags)
{
	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool CMetaObjectLanguage::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) {

		const wxString& strLangCode = m_propertyCode->GetValueAsString(); bool foundedName = false;
		const auto languageArray = m_metaData->GetAnyArrayObject<CMetaObjectLanguage>(g_metaLanguageCLSID);

		for (const auto object : languageArray) {
			if (object != this &&
				stringUtils::CompareString(strLangCode, object->GetCode())) {
				foundedName = true;
				break;
			}
		}

		if (foundedName) {
			const wxString& metaPrevName = m_propertyCode->GetValueAsString();
			size_t length = metaPrevName.length();
			while (length >= 0 && stringUtils::IsDigit(metaPrevName[--length]));
			for (unsigned int num_rec = 1;; num_rec++) {
				const wxString& strNewLangCode = wxString::Format(wxT("%s%i"),
					metaPrevName.Left(length + 1), num_rec); bool foundedCodeName = false;
				for (const auto object : languageArray) {
					if (object != this &&
						stringUtils::CompareString(strNewLangCode, object->GetCode())) {
						foundedCodeName = true;
						break;
					}
				}

				if (!foundedCodeName) {
					m_propertyCode->SetValue(strNewLangCode);
					break;
				}
			}
		}
	}

	return IMetaObject::OnAfterRunMetaObject(flags);
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