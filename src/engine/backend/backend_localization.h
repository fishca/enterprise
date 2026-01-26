#ifndef __BACKEND_LOCALIZATION_H__
#define __BACKEND_LOCALIZATION_H__

#include "backend_core.h"

struct CBackendLocalizationEntry {
	wxString m_code;
	wxString m_data;
};

typedef std::vector<CBackendLocalizationEntry> CBackendLocalizationEntryArray;

class BACKEND_API CBackendLocalization {
	CBackendLocalization() = delete;
public:

	static void SetUserLanguage(const wxString& strUserLanguage);
	static wxString GetUserLanguage();
	
	static bool CreateLocalizationArray(const wxString& strRawTranslate,
		CBackendLocalizationEntryArray& array);

	static wxString CreateLocalizationRawLocText(const wxString& strLocale);
	static bool IsLocalizationString(const wxString& strRawLocale);
	static wxString GetRawLocText(const CBackendLocalizationEntryArray& array);

	static bool IsEmptyLocalizationString(const wxString& strRawLocale);

	static void SetArrayTranslate(CBackendLocalizationEntryArray& array, const wxString &strResult);
	static void SetArrayTranslate(const wxString& strLangCode, CBackendLocalizationEntryArray& array, const wxString& strResult);

	static bool GetTranslateFromArray(const wxString& strLangCode,
		const CBackendLocalizationEntryArray& array, wxString& strResult);
	static wxString GetTranslateFromArray(const wxString& strLangCode,
		const CBackendLocalizationEntryArray& array);
	
	static bool GetTranslateGetRawLocText(
		const wxString& strRawLocale, wxString& strResult);
	static bool GetTranslateGetRawLocText(
		const wxString& strLangCode, const wxString& strRawLocale, wxString& strResult);

	static wxString GetTranslateGetRawLocText(const wxString& strRawLocale);
	static wxString GetTranslateGetRawLocText(const wxString& strLangCode, const wxString& strRawLocale);
};

#endif 