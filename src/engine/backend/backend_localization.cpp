#include "backend_localization.h"
#include "appData.h" 

static wxString ms_strEmptyLanguage;
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

bool CBackendLocalization::CreateLocalizationArray(const wxString& strRawLocale, CBackendLocalizationEntryArray& array)
{
	bool open_text = false,
		open_symbol = false;

	if (!IsLocalizationString(strRawLocale))
		return false;

#pragma region _calc_count_h_

	static size_t reserve_count = 0;

	for (const auto& c : strRawLocale.ToStdWstring()) {

		if ((!open_text && (c == wxT(' ') || c == wxT('\n'))) || c == wxT('\'')) {
			if (open_text && c == wxT('\''))
				open_symbol = !open_symbol;
			continue;
		}
		else if (!open_text && !open_symbol && c == wxT('=')) {
			open_text = true;
			continue;
		}
		else if (open_text && !open_symbol && c == wxT(';')) {
			reserve_count++;
			open_text = open_symbol = false;
			continue;
		}
	}

	open_text = open_symbol = false;

	array.clear();
	array.reserve(reserve_count);

	reserve_count = 0;

#pragma endregion

	static CBackendLocalizationEntry entry;

	for (const auto& c : strRawLocale.ToStdWstring()) {

		if ((!open_text && (c == wxT(' ') || c == wxT('\n'))) || c == wxT('\'')) {
			if (open_text && c == wxT('\''))
				open_symbol = !open_symbol;
			continue;
		}
		else if (!open_text && !open_symbol && c == wxT('=')) {
			open_text = true;
			continue;
		}
		else if (open_text && !open_symbol && c == wxT(';')) {
			const wxString& code = entry.m_code;
			const auto iterator = std::find_if(array.begin(), array.end(),
				[code](const CBackendLocalizationEntry& e) {
					return stringUtils::CompareString(code, e.m_code); });

			if (iterator == array.end()) {
				array.emplace_back(std::move(entry));
			}
			else {
				*iterator = std::move(entry);
			}

			open_text = open_symbol = false;
			continue;
		}

		if (open_text && open_symbol)
			entry.m_data += c;
		else if (!open_text && !open_symbol)
			entry.m_code += c;
	}

	return array.size() > 0;
}

wxString CBackendLocalization::CreateLocalizationRawLocText(const wxString& strLocale)
{
	static CBackendLocalizationEntryArray array;
	if (CreateLocalizationArray(strLocale, array))
		return GetTranslateFromArray(ms_strUserLanguage, array);

	return wxString::Format(wxT("%s = '%s';"),
		ms_strUserLanguage, strLocale);
}

bool CBackendLocalization::IsLocalizationString(const wxString& strRawLocale)
{
	if (strRawLocale.IsEmpty())
		return false;

	bool open_text = false,
		open_symbol = false;

	bool success = false;

	for (const auto& c : strRawLocale.ToStdWstring()) {

		if ((!open_text && (c == wxT(' ') || c == wxT('\n'))) || c == wxT('\'')) {
			if (open_text && c == wxT('\''))
				open_symbol = !open_symbol;
			success = false;
			continue;
		}
		else if (!open_text && !open_symbol && c == wxT('=')) {
			open_text = true;
			success = false;
			continue;
		}
		else if (open_text && !open_symbol && c == wxT(';')) {
			open_text = open_symbol = false;
			success = true;
			continue;
		}
	}

	return success;
}

wxString CBackendLocalization::GetRawLocText(const CBackendLocalizationEntryArray& array)
{
	static wxString strRawTranslate;
	for (const auto pair : array) {
		strRawTranslate += wxString::Format(
			wxT("%s = '%s';"), pair.m_code, pair.m_data);
	}
	return std::move(strRawTranslate);
}

bool CBackendLocalization::IsEmptyLocalizationString(const wxString& strRawLocale)
{
	if (strRawLocale.IsEmpty())
		return true;

	bool open_text = false,
		open_symbol = false;

	bool success = false;

	static wxString code, data;
	code.Clear(); data.Clear();

	for (const auto& c : strRawLocale.ToStdWstring()) {

		if ((!open_text && (c == wxT(' ') || c == wxT('\n'))) || c == wxT('\'')) {
			if (open_text && c == wxT('\''))
				open_symbol = !open_symbol;
			success = false;
			continue;
		}
		else if (!open_text && !open_symbol && c == wxT('=')) {
			open_text = true;
			success = false;
			continue;
		}
		else if (open_text && !open_symbol && c == wxT(';')) {		
			open_text = open_symbol = false;
			success = true;
			if (stringUtils::CompareString(ms_strUserLanguage, code))
				break;
			continue;
		}

		if (open_text && open_symbol)
			data += c;
		else if (!open_text && !open_symbol)
			code += c;
	}

	if (success && stringUtils::CompareString(ms_strUserLanguage, code))
		return data.IsEmpty();
	
	return true; 
}

bool CBackendLocalization::GetTranslateGetRawLocText(const wxString& strRawLocale, wxString& strResult)
{
	return GetTranslateGetRawLocText(ms_strUserLanguage, strRawLocale, strResult);
}

bool CBackendLocalization::GetTranslateGetRawLocText(const wxString& strLangCode, const wxString& strRawLocale, wxString& strResult)
{
	static CBackendLocalizationEntryArray array;
	if (CreateLocalizationArray(strRawLocale, array))
		return GetTranslateFromArray(strLangCode, array, strResult);
	
	strResult.Clear();
	return false;
}

wxString CBackendLocalization::GetTranslateGetRawLocText(const wxString& strRawLocale)
{
	static wxString strResult;
	if (GetTranslateGetRawLocText(strRawLocale, strResult))
		return std::move(strResult);
	return ms_strEmptyLanguage;
}

wxString CBackendLocalization::GetTranslateGetRawLocText(const wxString& strLangCode, const wxString& strRawLocale)
{
	static wxString strResult;
	if (GetTranslateGetRawLocText(strLangCode, strRawLocale, strResult))
		return std::move(strResult);
	return ms_strEmptyLanguage;
}

void CBackendLocalization::SetArrayTranslate(CBackendLocalizationEntryArray& array, const wxString& strResult)
{
	SetArrayTranslate(ms_strUserLanguage, array, strResult);
}

void CBackendLocalization::SetArrayTranslate(const wxString& strLangCode, CBackendLocalizationEntryArray& array, const wxString& strResult)
{
	for (auto& entry : array) {
		if (entry.m_code == strLangCode) {
			entry.m_data = strResult;
			break;
		}
	}
}

wxString CBackendLocalization::GetTranslateFromArray(const wxString& strLangCode, const CBackendLocalizationEntryArray& array)
{
	static wxString result;
	GetTranslateFromArray(strLangCode, array, result);
	return std::move(result);
}

bool CBackendLocalization::GetTranslateFromArray(const wxString& strLangCode, const CBackendLocalizationEntryArray& array, wxString& strResult)
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