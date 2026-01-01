#ifndef __BACKEND_LOCALIZATION_H__
#define __BACKEND_LOCALIZATION_H__

#include "backend_core.h"

struct CBackendLocalizationEntry {
	wxString m_code;
	wxString m_data;
};

class BACKEND_API CBackendLocalization {
	CBackendLocalization() = delete;
public:

	static void SetUserLanguage(const wxString& strUserLanguage);
	static wxString GetUserLanguage();
	
	static bool CreateLocalizationArray(const wxString& strRawTranslate,
		std::vector<CBackendLocalizationEntry>& array);
	static wxString CreateLocalizationRawLocText(const wxString& strLocale);

	static wxString GetRawLocText(
		const std::vector<CBackendLocalizationEntry>& array);

	static bool GetTranslateFromArray(const wxString& strLangCode,
		const std::vector<CBackendLocalizationEntry>& array, wxString& strResult);
	
	static wxString GetTranslateFromArray(const wxString& strLangCode,
		const std::vector<CBackendLocalizationEntry>& array) {
		wxString result;
		GetTranslateFromArray(strLangCode, array, result);
		return result;
	}

	static bool GetTranslateGetRawLocText(
		const wxString& strRawLocale, wxString& strResult) {
		const wxString& strUserLanguage = GetUserLanguage();
		return GetTranslateGetRawLocText(strUserLanguage, strRawLocale, strResult);
	}

	static bool GetTranslateGetRawLocText(
		const wxString& strLangCode, const wxString& strRawLocale, wxString& strResult) {
		std::vector<CBackendLocalizationEntry> array;
		if (CreateLocalizationArray(strRawLocale, array))
			return GetTranslateFromArray(strLangCode, array, strResult);
		return false;
	}

	static wxString GetTranslateGetRawLocText(const wxString& strRawLocale) {
		wxString strResult;
		if (GetTranslateGetRawLocText(strRawLocale, strResult))
			return strResult;
		return wxEmptyString;
	}

	static wxString GetTranslateGetRawLocText(const wxString& strLangCode, const wxString& strRawLocale) {
		wxString strResult;
		if (GetTranslateGetRawLocText(strLangCode, strRawLocale, strResult))
			return strResult;
		return wxEmptyString;
	}
};

#endif 