#include "constant.h"

/* XPM */
static const char *s_constantGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 30 1",
	"  c None",
	"# c #FFFFFF",
	"o c #97A3AF",
	"7 c #B5BEC5",
	"2 c #BDC3CA",
	", c #BDC3CB",
	"q c #8998A6",
	"5 c #F9F9FA",
	"9 c #A2ACB8",
	"; c #8595A3",
	"< c #FAFAFB",
	"& c #ABB5BE",
	"3 c #B5BDC5",
	"1 c #B5BDC6",
	". c #8192A1",
	"= c #8696A4",
	"8 c #8B9AA8",
	": c #95A2AE",
	"- c #A4AEB9",
	"X c #96A3AF",
	"0 c #A5AFBA",
	"4 c #BCC3CA",
	"> c #8394A2",
	"* c #FDFDFD",
	"O c #7F91A0",
	"6 c #FEFEFD",
	"$ c #A2ADB8",
	"@ c #FEFEFE",
	"% c #ACB5BE",
	"+ c #ACB5BF",
	/* pixels */
	"                ",
	" .XXXXXXXXXoO   ",
	" +@#########$   ",
	" %@.XXXXXXXXXoO ",
	" &*+@#########$ ",
	" =-%@;:oXXXXXXo>",
	"   &*,<########1",
	"   =-2<########3",
	"     45####**6@7",
	"     890------0q",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                "
};

/* XPM */
static const char *s_constant_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 3 1",
	"  c None",
	"X c #FFF5ED",
	". c #788B9C",
	/* pixels */
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"................",
	".XXXXXXXXXXXXXX.",
	".XXXXXXXXXXXXXX.",
	".XXXXXXXXXXXXXX.",
	".XXXXXXXXXXXXXX.",
	"................",
	"                ",
	"                ",
	"                ",
	"                ",
	"                "
};

wxIcon CMetaObjectConstant::GetIcon() const
{
	return wxIcon(s_constant_xpm);
}

wxIcon CMetaObjectConstant::GetIconGroup()
{
	return wxIcon(s_constant_xpm);
}