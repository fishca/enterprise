#include "metaAttributeObject.h"

static const char* s_attributeGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 27 1",
	"  c None",
	"$ c #DFF6FF",
	"5 c #94B1D4",
	"8 c #7E9EC8",
	": c #A6C1DD",
	"* c #E0F7FF",
	"4 c #9AB6D6",
	"+ c #7A9BC7",
	"X c #89A7CD",
	"O c #89A7CE",
	"3 c #D9F0FE",
	"< c #93AFD3",
	"- c #A8C3DE",
	"o c #8AA8CE",
	"; c #DAF1FE",
	"6 c #94B0D3",
	"> c #D6EEFC",
	"# c #DBF2FE",
	"2 c #DBF2FF",
	"7 c #95B1D4",
	"1 c #DCF3FF",
	"% c #DDF4FF",
	"= c #9CB7D7",
	"@ c #A9C3DE",
	"& c #DEF5FF",
	". c #7D9DC8",
	", c #82A1CA",
	/* pixels */
	"                ",
	"                ",
	" .XoOOOOOOo+    ",
	" @#$%%%%%&*=    ",
	" -;.XoOOOOOOo+  ",
	" :>@#$%%%%%&*=  ",
	" ,<-;.XoOOOOOOo+",
	"   :>@#$%%%%%&*=",
	"   ,<-;&11111%*=",
	"     :>233333;14",
	"     ,<566666678",
	"                ",
	"                ",
	"                ",
	"                ",
	"                "
};

/* XPM */
static const char *s_attribute_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 3 1",
	"  c None",
	". c #7496C4",
	"X c #DBF2FF",
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

wxIcon CMetaObjectAttribute::GetIcon() const
{
	return wxIcon(s_attribute_xpm);
}

wxIcon CMetaObjectAttribute::GetIconGroup()
{
	return wxIcon(s_attribute_xpm);
}