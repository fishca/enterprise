#include "metaDimensionObject.h"

/* PNG */
static const wxString s_dimension_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAH9s4D/38FifTNXAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectDimension::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectDimension::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_dimension_16_png, wxSize(16, 16));

	return icon;
}