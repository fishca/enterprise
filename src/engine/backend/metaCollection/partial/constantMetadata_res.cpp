#include "constant.h"

/* PNG */
static const wxString s_constant_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAACVBMVEUAAAF4i5z/9e36LQ2XAAAAAXRSTlMAQObYZgAAABlJREFUGJVjYKAFYEQDDIxMKIA8AXRDaQEAUdAAmeJBSsoAAAAASUVORK5CYII=");

wxIcon CMetaObjectConstant::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CMetaObjectConstant::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_constant_16_png, wxSize(16, 16));
	
	return icon;
}