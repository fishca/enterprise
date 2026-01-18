#include "databaseQueryParser.h"

bool IsEmptyQuery(const wxString& strQuery)
{
	if (strQuery.IsEmpty())
		return false;

	for (auto& c : strQuery) {

		// Remove all query delimiting semi-colons
		if (c != wxT(' ') &&
			c != wxT(';'))
			return false;
	}

	return true;
}

wxArrayString ParseQueries(const wxString& strQuery)
{
	static wxArrayString arrString; arrString.Empty();
	static wxString strRaw; static bool bInQuote = false;

	for (const auto& c : strQuery) {

		strRaw += c;

		if (c == wxT('\'')) {
			bInQuote = !bInQuote;
		}
		else if (c == wxT(';') && !bInQuote)
		{
			if (!IsEmptyQuery(strRaw))
				arrString.Add(strRaw);

			strRaw.Clear();
		}
	}

	if (!strRaw.IsEmpty()) {

		static wxString str; str.Clear();
		str << strRaw << wxT(";");
		if (!IsEmptyQuery(str))
			arrString.Add(str);

		strRaw.Clear();
	}

	return arrString;
}

