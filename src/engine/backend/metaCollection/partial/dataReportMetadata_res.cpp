#include "dataReport.h"

/* XPM */
static const char *s_reportGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 19 1",
	". c None",
	"o c #C94141",
	"% c #C54A4B",
	"X c #C54141",
	"; c #C84242",
	"- c #C83F3F",
	"+ c #C64242",
	"> c #C74343",
	"$ c #B0BFD2",
	": c #C84141",
	"* c #C64444",
	"  c #788B9C",
	"# c #C44141",
	", c #CC4C4C",
	"O c #C74242",
	"< c #C54545",
	"@ c #B0C1D3",
	"= c #C64343",
	"& c #C64040",
	/* pixels */
	" ...............",
	" ...............",
	" .............Xo",
	" ............OO+",
	" @.@.@#$.@.@O%&@",
	" ....*O=-..OO&..",
	" ...;+:OOOO=*...",
	" ..*+*..:O>X....",
	" .*+*....,X.....",
	" .><............",
	" @.@.@.@.@.@.@.@",
	" ...............",
	" ...............",
	" ...............",
	" ...............",
	"                "
};

/* XPM */
static const char *s_report_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 45 1",
	"  c None",
	"d c #517DB7",
	"f c #517DB8",
	"O c #FFFFFF",
	"g c #487FB6",
	"* c #7E8FA0",
	"t c #4F7AB6",
	"4 c #EFC8C8",
	": c #CB5252",
	"w c #87B3EC",
	"j c #5078B5",
	"1 c #E5A8A8",
	"a c #517CB7",
	"7 c #E1E9F3",
	"< c #CA4D4D",
	"= c #FFFEFE",
	"5 c #CD5757",
	"0 c #9CB5D7",
	"& c #8D9DAC",
	"o c #00FFFF",
	"h c #4D79B4",
	"# c #F6F7F8",
	"3 c #CE5B5B",
	"q c #758A9D",
	". c #788B9C",
	"8 c #4E7AB5",
	"> c #CC5555",
	"p c #6791CC",
	"; c #D26969",
	"2 c #E09797",
	"s c #4F7BB4",
	"% c #7E90A0",
	"9 c #5983BB",
	"u c #8BB7F0",
	"+ c #8B9CAA",
	"r c #527CB5",
	", c #F5DDDD",
	"$ c #8E9DAC",
	"- c #EDC3C3",
	"@ c #7D91A0",
	"6 c #7D9DC9",
	"y c #7F7F7F",
	"i c #719ED7",
	"X c #7D8E9F",
	"e c #6C97CF",
	/* pixels */
	" .........Xo    ",
	" .OOOOOOO.+@    ",
	" .OOOOOOO.#$%   ",
	" .OOOOOOO.O#&*  ",
	" .OOOOOOO.....  ",
	" .OOOOOOOOO=O.  ",
	" .OOOOOOOO-;O.  ",
	" .OOO-:-O->,O.  ",
	" .OO-<1<2>,OO.  ",
	" .OO34O45,67O.  ",
	" .OOOOOOOO890q  ",
	" .OOOOOOOO8werty",
	" .OOOOOOOO8uui88",
	" .OOOOOOOO8wpasy",
	" .........8dfg  ",
	"          hj    "
};

wxIcon CMetaObjectReport::GetIcon() const
{
	return wxIcon(s_report_xpm);
}

wxIcon CMetaObjectReport::GetIconGroup()
{
	return wxIcon(s_report_xpm);
}
