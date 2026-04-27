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

	// Process-wide configuration-language default. Set once during
	// CreateAppDataEnv from the platform locale; later auth flows route
	// through SetActiveLanguage (per-session) instead so each web tab can
	// render its own user's language without overwriting peers.
	// Get is by-const-ref because it sits on the hot translate path
	// (callers that only want the process default skip GetActiveLanguage's
	// session check).
	static void SetUserLanguage(const wxString& strUserLanguage);
	static const wxString& GetUserLanguage();

	// Active configuration-language for the calling thread — picks the
	// scoped session's GetLanguageCode() if any (per-tab), otherwise the
	// process-wide default. Every internal lookup (synonym translate,
	// raw-loc encode/decode) routes through here.
	//
	// HOT PATH. A single report line / form synonym lookup hits this once
	// per translatable field, multiplied by row count — easily millions
	// of calls on a 10k-row report. The implementation must stay at
	// (const wxString&) return + cached session-side value + no logic per
	// call. Do not let this slip back to by-value or to per-call fallback
	// chains.
	static const wxString& GetActiveLanguage();

	// Write configuration-language for the calling context. If a session
	// is bound on this thread, writes onto it (per-session); otherwise
	// updates the process-wide default. Use during auth handlers and
	// designer property-change sites — keeps the per-session vs global
	// decision in one place.
	static void SetActiveLanguage(const wxString& strLanguage);
	
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