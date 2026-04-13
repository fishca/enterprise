#include "metadata.h"

wxString IMetaData::Serialize(const CValue& cValue)
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

CValue IMetaData::Deserialize(const wxString& strValue)
{
	CValue createdValue;

	wxStringTokenizer str(wxT(";;;"));

	try {
		createdValue = IMetaData::CreateObject(g_valueStringCLSID);
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