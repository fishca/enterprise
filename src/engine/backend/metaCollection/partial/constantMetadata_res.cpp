#include "constant.h"

/* PNG */
static const wxString s_constant_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAF4i5z/9e36LQ2XAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon ibValueMetaObjectConstant::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueMetaObjectConstant::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_constant_16_png, wxSize(16, 16));
	
	return icon;
}