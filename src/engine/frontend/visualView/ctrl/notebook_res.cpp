#include "notebook.h"

/* XPM */
static char* s_notebook_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 6 1",
	"X c None",
	"  c #FFFFFF",
	"O c #989898",
	". c #708CBD",
	"+ c #DDE2EC",
	"o c #D8D8D8",
	/* pixels */
	"     .   .   .XX",
	" oooo.OOO.OOO.XX",
	" oooo.OOO.OOO.XX",
	" oooo.OOO.OOO.XX",
	" ++++          .",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ++++++++++++++.",
	" ...............",
	"XXXXXXXXXXXXXXXX"
};

wxIcon CValueNotebook::GetIcon() const
{
	return wxIcon(s_notebook_xpm);
}

wxIcon CValueNotebook::GetIconGroup()
{
	return wxIcon(s_notebook_xpm);
}