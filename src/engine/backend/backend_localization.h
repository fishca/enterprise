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

	// Process-wide configuration-language default. Pinned by metadata
	// OnInitialize to the configuration's main language code (the
	// metadata short-code form ru/en/uk that localization arrays are
	// keyed on); set once at boot from the platform locale before
	// metadata loads.
	static void SetUserLanguage(const wxString& strUserLanguage);

	// Active configuration-language for the calling thread — session's
	// GetLanguageCode() if a session is bound and it has a code,
	// otherwise the process-wide default above. Every internal lookup
	// (synonym translate, raw-loc encode/decode) and the designer's
	// advprop string editor route through here.
	//
	// HOT PATH. A single report line / form synonym lookup hits this
	// once per translatable field, multiplied by row count — easily
	// millions of calls on a 10k-row report. The implementation must
	// stay at (const wxString&) return + cached session-side value +
	// no logic per call.
	static const wxString& GetUserLanguage();

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