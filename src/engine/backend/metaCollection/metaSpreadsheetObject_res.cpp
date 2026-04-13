#include "metaSpreadsheetObject.h"

/* PNG */
static const wxString s_grid_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAsVBMVEUAAAB+kqOMnqmSo66VpbCOn6uNnqqOn6qPoKuRoq2WpbCisLqptr+8xs2Bk6N4i5x3ipuFlqWZp7SbqbaRoa+NnqyMnayMnqyNnq2QoK6TorCVpLKXprOaqLX29/j////+/v6wuL2OmaHz9Pbw8vP9/v7fhXrRUED8/f7t7/H7/P34+vzs7vD19vf5+/zz9/ro6+3u9Pjr8vfw8vT1+fvm6ezs7/GEmKaOnquNnquPn6yVhWyxAAAAAXRSTlMAQObYZgAAAJVJREFUGJVlj4kOgjAMhovHvEHmOQ8KU8ELPBH1/R/MbiSwyJcmf/O1WTOAf6xavdFkjLXanW6vP7DAdoihhsKxweUjPp5MZ3OxWK7W3AXhoYEnlPADiZIqQNxogTIfU2yV2FEfRlFIsT8ocSw3TmcScUKd3pB4uWrhF0duSnhJeeX+yN8oeMYk0pdBKiB7fwy+WeX3P2NWHAQu9L7mAAAAAElFTkSuQmCC");

wxIcon IValueMetaObjectSpreadsheet::GetIcon() const
{
	return GetIconGroup();
}

wxIcon IValueMetaObjectSpreadsheet::GetIconGroup()
{
	static wxIcon icon = 
		CBackendPicture::GetIconFromBase64(s_grid_16_png, wxSize(16, 16));

	return icon;
}