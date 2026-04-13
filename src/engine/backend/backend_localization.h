#ifndef __BACKEND_LOCALIZATION_H__
#define __BACKEND_LOCALIZATION_H__

#include "backend_core.h"

struct ibBackendLocalizationEntry {
	wxString m_code;
	wxString m_data;
};

typedef std::vector<ibBackendLocalizationEntry> ibBackendLocalizationEntryArray;

class BACKEND_API ibBackendLocalization {
	ibBackendLocalization() = delete;
public:

	static void SetUserLanguage(const wxString& strUserLanguage);
	static wxString GetUserLanguage();
	
	static bool CreateLocalizationArray(const wxString& strRawTranslate,
		ibBackendLocalizationEntryArray& array);

	static wxString CreateLocalizationRawLocText(const wxString& strLocale);
	static bool IsLocalizationString(const wxString& strRawLocale);
	static wxString GetRawLocText(const ibBackendLocalizationEntryArray& array);
	static bool GetRawLocText(const ibBackendLocalizationEntryArray& array, wxString& strResult);

	static bool IsEmptyLocalizationString(const wxString& strRawLocale);

	static void SetArrayTranslate(ibBackendLocalizationEntryArray& array, const wxString &strResult);
	static void SetArrayTranslate(const wxString& strLangCode, ibBackendLocalizationEntryArray& array, const wxString& strResult);

	static bool GetTranslateFromArray(const wxString& strLangCode,
		const ibBackendLocalizationEntryArray& array, wxString& strResult);
	static wxString GetTranslateFromArray(const wxString& strLangCode,
		const ibBackendLocalizationEntryArray& array);
	
	static bool GetTranslateGetRawLocText(
		const wxString& strRawLocale, wxString& strResult);
	static bool GetTranslateGetRawLocText(
		const wxString& strLangCode, const wxString& strRawLocale, wxString& strResult);

	static wxString GetTranslateGetRawLocText(const wxString& strRawLocale);
	static wxString GetTranslateGetRawLocText(const wxString& strLangCode, const wxString& strRawLocale);
};

#endif 