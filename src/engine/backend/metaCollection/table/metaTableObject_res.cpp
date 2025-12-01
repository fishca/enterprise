#include "metaTableObject.h"

/* XPM */
static const char* s_tableGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 4 1",
	"  c None",
	"o c #DBF2FF",
	". c #4788C7",
	"X c #98CCFD",
	/* pixels */
	"                ",
	"................",
	".XXXX.XXXXXXXXX.",
	".XXXX.XXXXXXXXX.",
	".XXXX.XXXXXXXXX.",
	"................",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	"................",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	"................",
	"                "
};

/* XPM */
static const char* s_table_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 4 1",
	"  c None",
	"o c #DBF2FF",
	". c #4788C7",
	"X c #98CCFD",
	/* pixels */
	"                ",
	"................",
	".XXXX.XXXXXXXXX.",
	".XXXX.XXXXXXXXX.",
	".XXXX.XXXXXXXXX.",
	"................",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	"................",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	".oooo.ooooooooo.",
	"................",
	"                "
};

wxIcon CMetaObjectTableData::GetIcon() const
{
	return wxIcon(s_table_xpm);
}

wxIcon CMetaObjectTableData::GetIconGroup()
{
	return wxIcon(s_table_xpm);
}
