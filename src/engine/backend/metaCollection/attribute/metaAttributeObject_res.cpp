#include "metaAttributeObject.h"

/* PNG */
static const wxString s_attribute_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAF0lsTb8v+YmrAhAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectAttribute::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectAttribute::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_attribute_16_png, wxSize(16, 16));

	return icon;
}