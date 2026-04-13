#include "metaResourceObject.h"

/* PNG */
static const wxString s_resource_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAGTwVjb8v/uftbcAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectResource::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectResource::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_resource_16_png, wxSize(16, 16));

	return icon;
}
