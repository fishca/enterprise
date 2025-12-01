#include "enumeration.h"

/* XPM */
static const char *s_enumGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 19 1",
	"  c None",
	"; c #697896",
	": c #65788E",
	"# c #65788F",
	". c #71718D",
	"@ c #667F99",
	"$ c #66798E",
	"o c #66798F",
	"> c #64768E",
	"< c #6D7F91",
	"% c #66788D",
	"- c #66788E",
	"+ c #66788F",
	"* c #0000FF",
	", c #677991",
	"X c #65798E",
	"O c #65798F",
	"= c #667788",
	"& c #647A8F",
	/* pixels */
	"                ",
	"  .XoX    Oo+@  ",
	"  +o+#    $+oX  ",
	"  Oo%      &oO  ",
	"  +#*      *+$  ",
	"  +$        +$  ",
	" =o-        +o; ",
	" :o-        -oo ",
	" ooX        +o- ",
	" =o+        -#= ",
	"  oo        :o  ",
	"  o-        o:  ",
	"  oo>      ,oo  ",
	"  :oO+    #Ooo  ",
	"  <:O+    $O-=  ",
	"                "
};

/* XPM */
static const char* s_enum_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 28 1",
	"  c None",
	"8 c #87878D",
	". c #FFFFFF",
	"* c #888B8B",
	"> c #888888",
	"; c #88888B",
	"9 c #88888E",
	"O c #848E8E",
	"@ c #89898B",
	"3 c #89898C",
	"7 c #89898D",
	"1 c #919191",
	"X c #8A8A8A",
	"& c #8A8A8C",
	"- c #8A8A8D",
	"4 c #888A8A",
	"= c #888A8B",
	"$ c #888A8C",
	"o c #8B8B8B",
	"# c #878B8B",
	", c #8C8C8C",
	"5 c #858590",
	"2 c #858591",
	"< c #88898B",
	"+ c #88898C",
	"6 c #898A8B",
	": c #7F7F7F",
	"% c #898A8C",
	/* pixels */
	"   .Xo    X.    ",
	"  O+@#    +$X   ",
	"  $%&     *%@   ",
	"  =%-     -%=   ",
	"  =%-     -%=   ",
	"  %%;     ;%%   ",
	" :%%>     ,<%1  ",
	"2=<3       4<+5 ",
	"5+<4 % % % 3<62 ",
	" 1<<,     ,<%1  ",
	"  %%;     ;%%   ",
	"  =%-     -%=   ",
	"  =%-     -%=   ",
	"  @%*     3%$   ",
	"  X$+7    635   ",
	"   .89    X.    "
};

wxIcon CMetaObjectEnumeration::GetIcon() const
{
	return wxIcon(s_enum_xpm);
}

wxIcon CMetaObjectEnumeration::GetIconGroup()
{
	return wxIcon(s_enum_xpm);
}
