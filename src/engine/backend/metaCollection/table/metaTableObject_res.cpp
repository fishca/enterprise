#include "metaTableObject.h"

/* PNG */
static const wxString s_table_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAADFBMVEUAAAFHiMeYzP3b8v/KNOThAAAAAXRSTlMAQObYZgAAAChJREFUGJVjYEAHjGiAgZEJCMAEGBAlgGEGMxCACTAgT4AahqKbgQ4A4aYBvhxdShsAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectTableData::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectTableData::GetIconGroup()
{
	static wxIcon icon = 
		CBackendPicture::GetIconFromBase64(s_table_16_png, wxSize(16, 16));
	
	return icon;
}
