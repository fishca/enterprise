#include "metaLanguageObject.h"

//***********************************************************************
//*                            MetaObjectLanguage                       *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectLanguage, IValueMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

#include "backend/metadata.h"

bool CValueMetaObjectLanguage::IsValidCode(const wxString& strLangCode)
{
	if (m_metaData == nullptr)
		return false;

	for (const auto language : m_metaData->GetAnyArrayObject<CValueMetaObjectLanguage>(g_metaLanguageCLSID)) {
		if (language != this && stringUtils::CompareString(strLangCode, language->GetLangCode()))
			return false;
	}

	return true;
}

CValueMetaObjectLanguage::CValueMetaObjectLanguage(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObject(name, synonym, comment)
{
}

bool CValueMetaObjectLanguage::OnDeleteMetaObject()
{
	const IValueMetaObject* commonObject = m_metaData->GetCommonMetaObject();

	return true;
}

bool CValueMetaObjectLanguage::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectLanguage::OnAfterRunMetaObject(int flags)
{
	if ((flags & newObjectFlag) != 0 || (flags & pasteObjectFlag) != 0) {

		const wxString& strLangCode = m_propertyCode->GetValueAsString(); bool foundedName = false;
		const auto languageArray = m_metaData->GetAnyArrayObject<CValueMetaObjectLanguage>(g_metaLanguageCLSID);

		for (const auto object : languageArray) {
			if (object != this &&
				stringUtils::CompareString(strLangCode, object->GetLangCode())) {
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
						stringUtils::CompareString(strNewLangCode, object->GetLangCode())) {
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

	return IValueMetaObject::OnAfterRunMetaObject(flags);
}

bool CValueMetaObjectLanguage::LoadData(CMemoryReader& reader)
{
	return m_propertyCode->LoadData(reader);
}

bool CValueMetaObjectLanguage::SaveData(CMemoryWriter& writer)
{
	return m_propertyCode->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectLanguage, "Language", g_metaLanguageCLSID);