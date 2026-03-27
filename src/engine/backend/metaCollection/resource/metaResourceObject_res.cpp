#include "metaResourceObject.h"

/* PNG */
static const wxString s_resource_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAGTwVjb8v/uftbcAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon ibValueMetaObjectResource::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueMetaObjectResource::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_resource_16_png, wxSize(16, 16));

	return icon;
}
