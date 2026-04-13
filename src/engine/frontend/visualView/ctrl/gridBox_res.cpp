#include "gridBox.h"

/* XPM */
static char* s_grid_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 4 1",
	"  c Black",
	"o c #FFFFFF",
	"X c #C9C7C7",
	". c #6C77B5",
	/* pixels */
	"                ",
	" ...X......X... ",
	" XXXXXXXXXXXXXX ",
	" ...XooooooXooo ",
	" ...XooooooXooo ",
	" ...XooooooXooo ",
	" ...XooooooXooo ",
	" XXXXXXXXXXXXXX ",
	" ...XooooooXooo ",
	" XXXXXXXXXXXXXX ",
	" ...XooooooXooo ",
	" XXXXXXXXXXXXXX ",
	" ...XooooooXooo ",
	" ...XooooooXooo ",
	" ...XooooooXooo ",
	"                "
};

wxIcon CValueGridBox::GetIcon() const
{
	return wxIcon(s_grid_xpm);
}

wxIcon CValueGridBox::GetIconGroup()
{
	return wxIcon(s_grid_xpm);
}