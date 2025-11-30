#include "catalog.h"

/* XPM */
static const char *s_catalogGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 18 1",
	"o c None",
	"$ c #FFC49C",
	": c #FFFFFF",
	"X c #A16A4A",
	"> c #A77256",
	"# c #FFD5B8",
	"+ c #DE9E72",
	", c #A16C4B",
	"& c #AB7453",
	"O c #A46D4D",
	"= c #A26D4C",
	"@ c #DB996C",
	"- c #A26D4D",
	". c #A56E4D",
	"  c #A16B4D",
	"; c #F1E9E5",
	"% c #E3A984",
	"* c #A57050",
	/* pixels */
	" .XXXXXXXXXooooo",
	"O+@#$$$$$$Xooooo",
	"X@@#$$$$$$Xooooo",
	"X@@#$%&XXXXXXXXX",
	"X@@#$&+@#$$$$$$X",
	"X@@#$X@@#$$$$$$X",
	"X@@#$X@@#$$$$$$X",
	"X@@#$X@@#$$$$$$X",
	"X@@#$X@@#$$$$$$X",
	"X@@#$X@@#$$$$$$X",
	"*=XXXX@@#$$$$$$X",
	"-;:::X@@#$$$$$$X",
	">,XXXX@@#$$$$$$X",
	"ooooo*=XXXXXXXXX",
	"ooooo-;::::::::X",
	"ooooo>,XXXXXXXXX"
};

/* XPM */
static const char *s_catalog_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 16 1",
	"  c None",
	"$ c #FFC49C",
	"- c #FFFFFF",
	"o c #A16A4A",
	"; c #A77256",
	"# c #FFD5B8",
	"+ c #DE9E72",
	": c #A16C4B",
	"O c #A46D4D",
	"& c #A26D4C",
	"@ c #DB996C",
	"* c #A26D4D",
	"X c #A56E4D",
	". c #A16B4D",
	"= c #F1E9E5",
	"% c #A57050",
	/* pixels */
	" .Xoooooooooooo ",
	" O+@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" o@@#$$$$$$$$$o ",
	" %&oooooooooooo ",
	" *=-----------o ",
	" ;:oooooooooooo ",
	"                "
};

wxIcon CMetaObjectCatalog::GetIcon() const
{
	return wxIcon(s_catalog_xpm);
}

wxIcon CMetaObjectCatalog::GetIconGroup()
{
	return wxIcon(s_catalog_xpm);
}