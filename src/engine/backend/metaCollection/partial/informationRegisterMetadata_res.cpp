#include "informationRegister.h"

/* XPM */
static const char* s_informationRegisterGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 4 1",
	"  c None",
	"o c #DBF2FF",
	". c #4E7AB5",
	"X c #8BB7F0",
	/* pixels */
	"                ",
	"................",
	".XXX.oooooooooo.",
	".XXX.oooooooooo.",
	"................",
	"                ",
	"................",
	".XXXXXXXXXXX.oo.",
	".XXXXXXXXXXX.oo.",
	"................",
	"                ",
	"................",
	".XXXXXXX.oooooo.",
	".XXXXXXX.oooooo.",
	"................",
	"                "
};

/* XPM */
static const char* s_informationRegister_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 35 1",
	"  c None",
	"X c #68B48A",
	"% c #6CAB9C",
	"o c #6AAE95",
	"6 c #6AAE96",
	"w c #6EA5A7",
	"q c #6CA8A0",
	"e c #7496C4",
	"> c #C2E8FF",
	"3 c #C0E8F6",
	"2 c #C0E8F7",
	"1 c #C0E8F8",
	"# c #69B28E",
	"0 c #6BAC99",
	"* c #6BAC9A",
	"- c #BFE9F1",
	"4 c #6CAA9D",
	"& c #6CAA9E",
	"@ c #6AAD97",
	"$ c #69B190",
	"+ c #69B191",
	"9 c #6BAB9B",
	"7 c #6BAB9C",
	", c #C1E8FB",
	"t c #C1E8FD",
	"; c #BFE8F7",
	"r c #6AAF93",
	"y c #6AAF94",
	". c #68B28D",
	"< c #6CA99F",
	"= c #6AAC99",
	"5 c #BEE9EF",
	"O c #6BAD98",
	": c #69B092",
	"8 c #6DA7A2",
	/* pixels */
	"                ",
	" .XoO +O@# $%&* ",
	" =-;: O>,% <1,< ",
	" <12% *33: 45-6 ",
	" &678 7O<< 90qw ",
	"                ",
	" eeee O$or eeee ",
	" e>>e $327 e>>e ",
	" e>>e #,t6 e>>e ",
	" eeee yo+O eeee ",
	"                ",
	" eeee eeee eeee ",
	" e>>e e>>e e>>e ",
	" e>>e e>>e e>>e ",
	" eeee eeee eeee ",
	"                "
};

wxIcon CMetaObjectInformationRegister::GetIcon() const
{
	return wxIcon(s_informationRegister_xpm);
}

wxIcon CMetaObjectInformationRegister::GetIconGroup()
{
	return wxIcon(s_informationRegister_xpm);
}
