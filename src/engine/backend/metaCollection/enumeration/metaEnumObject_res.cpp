#include "metaEnumObject.h"

/* PNG */
static const wxString s_enumeration_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAGpVdfm3f/kjJsEAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectEnum::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectEnum::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_enumeration_16_png, wxSize(16, 16));

	return icon;
}