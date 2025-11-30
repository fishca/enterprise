#include "metaObjectMetadata.h"

/* XPM */
static const char *s_metadata_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 50 1",
	"  c None",
	"w c #EEE946",
	"h c #DEE8A3",
	"< c #D4E9DB",
	"2 c #D4E9DC",
	"1 c #DFE9A2",
	"y c #DFE9A3",
	". c #EFEA4C",
	"= c #DFE9A5",
	"r c #E2E788",
	"9 c #EAE968",
	"$ c #DDE9A5",
	"# c #EFEA62",
	"e c #FDE803",
	"k c #E8E35E",
	"t c #E0E79F",
	"& c #CEE9F9",
	"z c #DEEAA3",
	"o c #DEE79C",
	"O c #DEE79E",
	"f c #EEE848",
	"- c #E9E464",
	"i c #D4E8DD",
	"@ c #EFE94A",
	"; c #E7E773",
	"p c #E2E989",
	"d c #D2E8E2",
	"* c #D2E8E5",
	"% c #D2E8E6",
	"j c #D5E9CF",
	"6 c #EAE866",
	"> c #D5E9D2",
	": c #D5E9D3",
	"c c #E8E260",
	"+ c #E3E789",
	"X c #E3E78A",
	"x c #DEE9A2",
	"0 c #DEE9A4",
	"q c #D1E9E4",
	"a c #D1E9E5",
	"s c #E9E968",
	"g c #E9E969",
	"8 c #FEE800",
	"7 c #FEE802",
	"4 c #E9E361",
	"5 c #CFE9F3",
	"u c #DFE79D",
	"l c #E7E373",
	"3 c #D5E8D0",
	", c #EAE772",
	/* pixels */
	"     .XoO+@     ",
	"   #$%&&&&*=-   ",
	"  ;:&&&&&&&&>,  ",
	" #>&&&<112&&&34 ",
	" $&&56788795&&0 ",
	".*&&68888889&&qw",
	"X&&<7888888e2&&r",
	"t&&188888888y&&u",
	"u&&188888888y&&O",
	"+&&<e888888ei&&p",
	"@a&&9888888s&&df",
	" =&&59788eg5&&h ",
	" ->&&&21yi&&&jk ",
	"  ,3&&&&&&&&jl  ",
	"   4zq&&&&dxc   ",
	"     frOOpf     "
};

wxIcon CMetaObjectConfiguration::GetIcon() const
{
	return wxIcon(s_metadata_xpm);
}

wxIcon CMetaObjectConfiguration::GetIconGroup()
{
	return wxIcon(s_metadata_xpm);
}