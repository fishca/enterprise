#include "value.h"

/* PNG */
static const wxString s_value_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAD1BMVEUAAAF0lsR4i5zb8v////8M1jp0AAAAAXRSTlMAQObYZgAAAB9JREFUGJVjYKAFYIQDJjBgYGSGAkYWECBPAN1QWgAAfcgA7YtkuHgAAAAASUVORK5CYII=");

wxIcon CValue::GetIcon() const
{
	return GetIconGroup();
}

#include "backend/backend_picture.h"

wxIcon CValue::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_value_16_png, wxSize(16, 16));

	return icon;
}