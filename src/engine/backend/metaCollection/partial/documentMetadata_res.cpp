#include "document.h"

/* XPM */
static const char* s_documentGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 8 1",
	"X c None",
	"o c #FFFFFF",
	"@ c #8D9EAB",
	"# c #98A7B6",
	"+ c #94A4B3",
	"  c #788B9C",
	". c #91A2B0",
	"O c #8C9DAC",
	/* pixels */
	"        .XXXXXXX",
	" oooooo O+XXXXXX",
	" oooooo X@#XXXXX",
	" oooo        .XX",
	" oooo oooooo O+X",
	" oooo oooooo X@#",
	" oooo oooooo    ",
	" oooo ooooooooo ",
	" oooo oo     oo ",
	" oooo ooooooooo ",
	" oooo oo    ooo ",
	" oooo ooooooooo ",
	"      oo     oo ",
	"XXXXX ooooooooo ",
	"XXXXX ooooooooo ",
	"XXXXX           "
};

/* XPM */
static const char *s_document_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 11 1",
	"  c None",
	"o c #FFFFFF",
	"% c #8B9BA9",
	"+ c #7D909F",
	"@ c #F6F7F8",
	". c #788B9C",
	"X c #7D8F9F",
	"& c #7D8FA0",
	"# c #8E9DAC",
	"$ c #F4F6F7",
	"O c #8C9DAB",
	/* pixels */
	" ..........X    ",
	" .oooooooo.O+   ",
	" .oooooooo.@#X  ",
	" .oooooooo.o$%& ",
	" .ooooooo...... ",
	" .oooooooooooo. ",
	" .oooooooooooo. ",
	" .ooo......ooo. ",
	" .oooooooooooo. ",
	" .ooo....ooooo. ",
	" .oooooooooooo. ",
	" .ooo......ooo. ",
	" .oooooooooooo. ",
	" .oooooooooooo. ",
	" .............. ",
	"                "
};

wxIcon CMetaObjectDocument::GetIcon() const
{
	return wxIcon(s_document_xpm);
}

wxIcon CMetaObjectDocument::GetIconGroup()
{
	return wxIcon(s_document_xpm);
}
