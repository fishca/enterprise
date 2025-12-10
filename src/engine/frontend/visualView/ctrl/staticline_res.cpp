#include "widgets.h"

/* XPM */
static char* s_static_line_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 3 1",
	"  c None",
	"X c #FFFFFF",
	". c #ACA899",
	/* pixels */
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	" .............  ",
	"  XXXXXXXXXXXXX ",
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