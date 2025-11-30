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
	"16 16 43 1",
	"  c None",
	"1 c #1FC797",
	"$ c #00BF7F",
	"O c #78D3A7",
	"y c #6CD1A5",
	"* c #60CFA2",
	"p c #84D2A9",
	"& c #22C596",
	"r c #ADD6B1",
	"9 c #8ED4AB",
	"4 c #00FF7F",
	"f c #79D4A6",
	"t c #E0DCBA",
	"o c #7ED2A7",
	"e c #70D6A8",
	"0 c #51CEA2",
	"+ c #ACD7B0",
	"@ c #1FC994",
	": c #1FC998",
	"i c #9ED5AD",
	"q c #73D1A4",
	"8 c #B6D6B1",
	"6 c #22C797",
	"% c #22C799",
	"3 c #20CA95",
	"> c #00FFFF",
	"g c #68D3A5",
	". c #44CD9C",
	"d c #7BD0A6",
	"# c #1ECA96",
	"a c #55CD9F",
	"5 c #91D4AB",
	"s c #53D0A2",
	"< c #1FC896",
	"u c #84D3A8",
	"X c #84D3AC",
	"7 c #22C699",
	"= c #2AD4AA",
	"w c #87D1A9",
	", c #1EC998",
	"- c #21CA96",
	"; c #1CC69B",
	"2 c #24CE91",
	/* pixels */
	"            .Xo ",
	"             O+ ",
	"         @#$%&* ",
	"       =-;@:>   ",
	"   $,:<1>       ",
	"  234 =      55 ",
	" 67          88 ",
	" 4      980  88 ",
	"   qwe  rtyru88 ",
	"   itpasrtdti88 ",
	" ffitdtrrtdti88 ",
	" 88itdtrrtdti88 ",
	" 88itdtrrtdti88 ",
	" 88itdtrrtdti88 ",
	" 88itdtrrtdti88 ",
	" ffqwgwOOwgwqff "
};

wxIcon CMetaObjectReport::GetIcon() const
{
	return wxIcon(s_report_xpm);
}

wxIcon CMetaObjectReport::GetIconGroup()
{
	return wxIcon(s_report_xpm);
}
