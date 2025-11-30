#include "value.h"

/* XPM */
static const char* s_value_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 5 1",
	"  c None",
	"O c #FFFFFF",
	". c #7496C4",
	"o c #DBF2FF",
	"X c #788B9C",
	/* pixels */
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"..........XXXXXX",
	".oooooooo.OOOOOX",
	".oooooooo.OOOOOX",
	".oooooooo.OOOOOX",
	".oooooooo.OOOOOX",
	"..........XXXXXX",
	"                ",
	"                ",
	"                ",
	"                ",
	"                "
};

wxIcon CValue::GetIcon() const
{
	return wxIcon(s_value_xpm);
}

wxIcon CValue::GetIconGroup()
{
	return wxIcon(s_value_xpm);
}