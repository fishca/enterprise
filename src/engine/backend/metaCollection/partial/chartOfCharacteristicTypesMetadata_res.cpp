#include "chartOfCharacteristicTypes.h"

/* PNG - chart of characteristic types icon 16x16 (attribute-style, blue) */
static const wxString s_chartOfCharacteristicTypes_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAF0lsTb8v+YmrAhAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon ibValueMetaObjectChartOfCharacteristicTypes::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueMetaObjectChartOfCharacteristicTypes::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_chartOfCharacteristicTypes_16_png, wxSize(16, 16));

	return icon;
}
