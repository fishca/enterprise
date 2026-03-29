#include "valueTable.h"

/* PNG */
static const wxString s_table_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAADFBMVEUAAAFHiMeYzP3b8v/KNOThAAAAAXRSTlMAQObYZgAAAChJREFUGJVjYEAHjGiAgZEJCMAEGBAlgGEGMxCACTAgT4AahqKbgQ4A4aYBvhxdShsAAAAASUVORK5CYII=");

wxIcon ibValueModelTable::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueModelTable::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_table_16_png, wxSize(16, 16));

	return icon;
}