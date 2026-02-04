#include "document.h"

/* PNG */
static const wxString s_document_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAIVBMVEUAAAF4i5x9j5////+Mnat9kJ/29/iOnaz09veLm6l9j6C1uLjiAAAAAXRSTlMAQObYZgAAADhJREFUGJVjYGCEAyYGMGBkhgJGFlY0ATZ2JlQBZg5OLiQBiDnIKsCiuAQgygmpoNQMVAEUwIABAB6/AfGpRQdOAAAAAElFTkSuQmCC");

wxIcon CValueMetaObjectDocument::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectDocument::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_document_16_png, wxSize(16, 16));

	return icon;
}
