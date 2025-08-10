#include "databaseQueryParser.h"

bool IsEmptyQuery(const wxString& strQuery)
{
	if (strQuery.IsEmpty())
		return false;

	for (auto& c : strQuery) {

		// Remove all query delimiting semi-colons
		if (c != wxUniChar(' ') &&
			c != wxUniChar(';'))
			return false;
	}

	return true;
}

wxArrayString ParseQueries(const wxString& strQuery)
{
	wxArrayString returnArray; returnArray.Alloc(1); bool bInQuote = false;
	wxString strRaw; strRaw.Alloc(strQuery.Length());

	for (auto& c : strQuery) {

		strRaw += c;

		if (c == wxUniChar('\'')) {
			bInQuote = !bInQuote;
		}
		else if (c == wxUniChar(';') && !bInQuote)
		{
			if (!IsEmptyQuery(strRaw))
				returnArray.Add(strRaw);

			strRaw.Clear();
		}
	}

	if (!strRaw.IsEmpty()) {

		wxString str;
		str << strRaw << _T(";");
		if (!IsEmptyQuery(str))
			returnArray.Add(str);

		strRaw.Clear();
	}

	return returnArray;
}

