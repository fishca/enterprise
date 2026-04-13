#include "metadata.h"

wxString ibMetaData::Serialize(const ibValue& cValue)
{
	wxString strSerialize;
	if (cValue.Serialize(strSerialize)) {

		wxString str;

		str << wxT("S: OES Serialize;;;");
		str << wxT("C:") << cValue.GetClassType() << wxT(";;;");
		str << wxT("L:") << strSerialize.Length() << wxT(";;;");
		str << wxT("D:") << strSerialize << wxT(";;;");
		str << wxT("E: OES Serialize;;;");

		return str;
	}

	return wxEmptyString;
}

#include <wx/tokenzr.h>

ibValue ibMetaData::Deserialize(const wxString& strValue)
{
	ibValue createdValue;

	wxStringTokenizer str(wxT(";;;"));

	try {
		createdValue = ibMetaData::CreateObject(g_valueStringCLSID);
	}
	catch (...)
	{

	}

	if (createdValue.Deserialize(strValue))
	{
		return createdValue;
	}

	return wxEmptyValue;
}