#include "widgets.h"

/* XPM */
static char* s_static_line_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 4 1",
	"  c None",
	"o c #FFFFFF",
	"X c #ACA899",
	/* pixels */
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	" XXXXXXXXXXXXXX ",
	"  oooooooooooooo",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                "
};

wxIcon CValueStaticLine::GetIcon() const
{
	return wxIcon(s_static_line_xpm);
}

wxIcon CValueStaticLine::GetIconGroup()
{
	return wxIcon(s_static_line_xpm);
}