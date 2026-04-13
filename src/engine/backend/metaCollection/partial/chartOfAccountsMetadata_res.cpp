#include "chartOfAccounts.h"

/* PNG - chart of accounts icon 16x16 (dimension-style, orange) */
static const wxString s_chartOfAccounts_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAH9s4D/38FifTNXAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon ibValueMetaObjectChartOfAccounts::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueMetaObjectChartOfAccounts::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_chartOfAccounts_16_png, wxSize(16, 16));
	return icon;
}
