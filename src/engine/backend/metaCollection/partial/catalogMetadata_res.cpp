#include "catalog.h"

/* PNG */
static const wxString s_catalog_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAMFBMVEUAAAGha02lbk2hakqkbU3ennLbmWz/1bj/xJylcFCibUyibU3x6eX///+nclahbEvzIaCgAAAAAXRSTlMAQObYZgAAAC9JREFUGJVjYGBkYkYCDAwMLKxs7BwwABJgZhsqApxcaH7h5uFFAJAAHz+aCjQAAPm1BXSM41l5AAAAAElFTkSuQmCC");

wxIcon CValueMetaObjectCatalog::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectCatalog::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_catalog_16_png, wxSize(16, 16));

	return icon;
}