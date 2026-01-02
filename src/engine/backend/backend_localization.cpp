#include "backend_localization.h"
#include "appData.h" 

static wxString ms_strUserLanguage = wxT("en");

////////////////////////////////////////////////////////////////////////////////

void CBackendLocalization::SetUserLanguage(const wxString& strUserLanguage)
{
	if (strUserLanguage.IsEmpty()) {
		ms_strUserLanguage = wxT("en");
		return;
	}

	ms_strUserLanguage = strUserLanguage;
}

wxString CBackendLocalization::GetUserLanguage()
{
	return ms_strUserLanguage;
}

////////////////////////////////////////////////////////////////////////////////

bool CBackendLocalization::CreateLocalizationArray(const wxString& strRawLocale,
	std::vector<CBackendLocalizationEntry>& array)
{
	bool openText = false,
		openSymbol = false;

	CBackendLocalizationEntry entry;

	for (const auto c : strRawLocale) {

		if ((!openText && (c == wxT(' ') || c == wxT('\n'))) || c == wxT('\'')) {
			if (openText && c == wxT('\''))
				openSymbol = !openSymbol;
			continue;
		}
		else if (!openText && !openSymbol && c == wxT('=')) {
			openText = true;
			continue;
		}
		else if (openText && !openSymbol && c == wxT(';')) {

			auto iterator = std::find_if(array.begin(), array.end(),
				[entry](const CBackendLocalizationEntry& e) {
					return stringUtils::CompareString(e.m_code, entry.m_code); });

			if (iterator == array.end()) {
				array.emplace_back(std::move(entry));
			}
			else {
				*iterator = std::move(entry);
			}

			openText = openSymbol = false;
			continue;
		}

		if (openText && openSymbol)
			entry.m_data += c;
		else if (!openText && !openSymbol)
			entry.m_code += c;
	}

	return array.size() > 0;
}

wxString CBackendLocalization::CreateLocalizationRawLocText(const wxString& strLocale)
{
	std::vector<CBackendLocalizationEntry> array;
	if (CreateLocalizationArray(strLocale, array))
		return GetTranslateFromArray(ms_strUserLanguage, array);

	return wxString::Format(wxT("%s = '%s';"),
		ms_strUserLanguage, strLocale);
}

wxString CBackendLocalization::GetRawLocText(const std::vector<CBackendLocalizationEntry>& array)
{
	wxString strRawTranslate;
	for (const auto pair : array) {
		strRawTranslate += wxString::Format(
			wxT("%s = '%s';"), pair.m_code, pair.m_data);
	}

	return strRawTranslate;
}

bool CBackendLocalization::GetTranslateFromArray(const wxString& strLangCode, const std::vector<CBackendLocalizationEntry>& array, wxString& strResult)
{
	if (!strLangCode.IsEmpty()) {
		auto iterator = std::find_if(array.begin(), array.end(),
			[strLangCode](const CBackendLocalizationEntry& entry) {
				return stringUtils::CompareString(entry.m_code, strLangCode); });

		if (iterator != array.end()) {
			strResult = iterator->m_data;
			return true;
		}
		else if (!stringUtils::CompareString(ms_strUserLanguage, strLangCode)) {
			const wxString& strUserLanguage = ms_strUserLanguage;
			const auto iterator_by_default_lang = std::find_if(array.begin(), array.end(),
				[strUserLanguage](const CBackendLocalizationEntry& entry) {
					return stringUtils::CompareString(entry.m_code, strUserLanguage); });
			if (iterator_by_default_lang != array.end()) {
				strResult = iterator_by_default_lang->m_data;
				return true;
			}
		}
	}
	else {
		const wxString& strUserLanguage = ms_strUserLanguage;
		const auto iterator_by_default_lang = std::find_if(array.begin(), array.end(),
			[strUserLanguage](const CBackendLocalizationEntry& entry) {
				return stringUtils::CompareString(entry.m_code, strUserLanguage); });
		if (iterator_by_default_lang != array.end()) {
			strResult = iterator_by_default_lang->m_data;
			return true;
		}
	}

	strResult.Clear();
	return false;
}