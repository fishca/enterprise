#include "databaseStringConverter.h"

static wxCriticalSection s_dbStringConverterCS;
static wxCSConv g_def_encoding = wxT("UTF-8");

// Default the encoding converter
CDatabaseStringConverter::CDatabaseStringConverter()
	: m_Encoding(nullptr)
{
}

CDatabaseStringConverter::CDatabaseStringConverter(const CDatabaseStringConverter& src)
	: m_Encoding(nullptr)
{
	if (src.m_Encoding != nullptr) {
		m_Encoding = new wxCSConv(*src.m_Encoding);
	}
}

void CDatabaseStringConverter::SetEncoding(const wxCSConv* conv)
{
	if (conv != &g_def_encoding) {

		wxCriticalSectionLocker enter(s_dbStringConverterCS);

		if (m_Encoding != nullptr)
			wxDELETE(m_Encoding);

		m_Encoding = new wxCSConv(*conv);
	}
}

const wxCSConv* CDatabaseStringConverter::GetEncoding()
{
	if (m_Encoding == nullptr)
		return &g_def_encoding;

	return m_Encoding;
}

const wxCharBuffer CDatabaseStringConverter::ConvertToUnicodeStream(const wxString& inputString)
{
#if wxUSE_UNICODE
	return wxConvUTF8.cWC2MB(inputString.wc_str(*wxConvCurrent));
#else
	wxString str(inputString.wc_str(*wxConvCurrent), wxConvUTF8);
	return str.mb_str();
#endif
}

unsigned int CDatabaseStringConverter::GetEncodedStreamLength(const wxString& inputString)
{
#if wxUSE_UNICODE
	unsigned int length = wxConvUTF8.WC2MB(nullptr, inputString.wc_str(), (unsigned int)0);
#else
	wxString str(inputString.wc_str(*wxConvCurrent), wxConvUTF8);
	unsigned int length = str.Length();
#endif
	if (length == 0)
	{
		wxCharBuffer tempCharBuffer = ConvertToUnicodeStream(inputString);
		length = wxStrlen((wxChar*)(const char*)tempCharBuffer);
	}

	return length;
}

wxString CDatabaseStringConverter::ConvertFromUnicodeStream(const char* inputBuffer)
{
	wxString strReturn(wxConvUTF8.cMB2WC(inputBuffer), *wxConvCurrent);

	// If the UTF-8 conversion didn't return anything, then try the default unicode conversion
	if (strReturn == wxEmptyString)
		strReturn << wxString(inputBuffer, *wxConvCurrent);

	return strReturn;
}

const wxCharBuffer CDatabaseStringConverter::ConvertToUnicodeStream(const wxString& inputString, const char* encoding)
{
#if wxUSE_UNICODE
	return wxConvUTF8.cWC2MB(inputString.wc_str(*wxConvCurrent));
#else
	wxString str(inputString.wc_str(*wxConvCurrent), wxConvUTF8);
	return str.mb_str();
#endif
}

unsigned int CDatabaseStringConverter::GetEncodedStreamLength(const wxString& inputString, const char* encoding)
{
#if wxUSE_UNICODE
	unsigned int length = wxConvUTF8.WC2MB(nullptr, inputString.wc_str(), (unsigned int)0);
#else
	const wchar_t* str = inputString.wc_str(*wxConvCurrent);
	unsigned int length = wxConvUTF8.WC2MB(nullptr, str, (unsigned int)0);
#endif
	if (length == 0)
	{
		wxCharBuffer tempCharBuffer = CDatabaseStringConverter::ConvertToUnicodeStream(inputString, encoding);
		length = wxStrlen((wxChar*)(const char*)tempCharBuffer);
	}

	return length;
}

wxString CDatabaseStringConverter::ConvertFromUnicodeStream(const char* inputBuffer, const char* encoding)
{
	wxString strReturn(wxConvUTF8.cMB2WC(inputBuffer), *wxConvCurrent);

	// If the UTF-8 conversion didn't return anything, then try the default unicode conversion
	if (strReturn == wxEmptyString)
		strReturn << wxString(inputBuffer, *wxConvCurrent);

	return strReturn;
}

