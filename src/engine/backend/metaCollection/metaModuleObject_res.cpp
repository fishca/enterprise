#include "metaModuleObject.h"

/* XPM */
static const char* s_moduleGroup_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 113 2",
	";  c #BED7F8",
	"X  c None",
	"c  c #E8EEF8",
	"<. c #A0ADB8",
	"%  c #EDEFF1",
	" . c #F7FAFE",
	"~  c #F2F6FD",
	"*. c #F7FAFF",
	"W  c #D69DB6",
	"X. c #F0F9FF",
	"r  c #94A5B1",
	"f  c #94A2AE",
	"3  c #EBF2FC",
	"H  c #F5FAFF",
	"x  c #F5F7FA",
	"-. c #FAFBFC",
	"=  c #FFFFFF",
	"J  c #EEF9FF",
	"^  c #9CAAB6",
	"j  c #F8FBFE",
	"N  c #F3F7FE",
	"Y  c #CED5DA",
	"l  c #C9D1D9",
	"s  c #909FAD",
	"2  c #ECF3FC",
	"4  c #ECF3FD",
	"V  c #C2D9F8",
	"E  c #CEA0BC",
	"n  c #FBFCFE",
	"2. c #C2CAD0",
	"`  c #E5EFFC",
	"q  c #8E9FAF",
	"&  c #EAEDF1",
	"D  c #CCCCD2",
	"U  c #F9F9FA",
	"#  c #B1BBC4",
	"}  c #D6DAEE",
	"h  c #8798A6",
	"]  c #D8CEE2",
	".. c #F7FCFF",
	"d  c #91A0AE",
	"#. c #E5A8B2",
	"B  c #C3DAF8",
	"<  c #FCFDFD",
	"F  c #FCFDFF",
	"!  c #C7BFDC",
	"7  c #F5F9FE",
	"P  c #F5F6F7",
	"Z  c #BCC4CC",
	"   c #7E91A1",
	"&. c #D5E4FA",
	"O  c #8395A5",
	";. c #F8FAFB",
	"=. c #FDFEFD",
	">  c #BDD7F8",
	"v  c #E7EEF8",
	"g  c #95A2AE",
	"A  c #F6FAFF",
	"i  c #F1F6FE",
	"/  c #B8C1C8",
	"u  c #7A8E9D",
	"b  c #B8C1C9",
	"K  c #EFF9FF",
	"*  c #BDC5CD",
	"y  c #8496A4",
	"@  c #8496A5",
	"{  c #DAD0E2",
	"a  c #98A3B0",
	"%. c #D4E5FC",
	"'  c #EAE9F3",
	"0  c #8C9EAE",
	"O. c #F2F7FE",
	"5  c #EDF3FD",
	"[  c #DDCEDF",
	"z  c #F7F8F9",
	".  c #7B8F9D",
	")  c #B9C2C9",
	"(  c #B9C2CA",
	"S  c #E6F2FF",
	"L  c #F0F7FF",
	"@. c #EFDEE4",
	"$  c #EBEDEF",
	"G  c #F5F8FD",
	"Q  c #D2A4BE",
	"T  c #DAEAFC",
	"w  c #8D9FAF",
	"e  c #8D9FB0",
	"o  c #8394A4",
	"k  c #EEF4FC",
	"$. c #E2BDCB",
	"-  c #F3F8FF",
	"C  c #C4DAF8",
	"I  c #F8F9FA",
	",. c #F8F9FB",
	"6  c #FDFDFD",
	"_  c #BAC3CA",
	"8  c #E7F0FC",
	"o. c #F1F8FF",
	":  c #BDD6F8",
	"|  c #F6F9FE",
	"1  c #FBFDFF",
	"+. c #F6F9FF",
	"9  c #AEBBC8",
	"p  c #DBEBFD",
	"R  c #E4DAE7",
	"+  c #8495A5",
	"M  c #FEFEFE",
	",  c #BBC4CB",
	":. c #F9F7F7",
	">. c #F9F7F8",
	"1. c #C0C8CF",
	"m  c #CFD4DB",
	"t  c #96A5B1",
	/* pixels */
	"  . . . . . . . . . .   X X X X ",
	"o O + @ @ @ @ @ @ + O o X X X X ",
	"# $ % & & & & & & % $ # X X X X ",
	"* = - ; : > > : ; - = * X X X X ",
	", < 1 2 3 4 4 4 5 1 < , X X X X ",
	", 6 7 8 9 0 q w e r t y u u u   ",
	", 6 i p a s d f g g g g f d s h ",
	", 6 j k l z x c v v v v c x z b ",
	", < n 7 m M N B V B B V C N M Z ",
	"Z M A S D < F G H J K K L F < , ",
	"b P I U Y 6 i T R E W Q ! ~ 6 , ",
	"^ / ( ) _ 6 7 ` ' ] [ { } | 6 , ",
	"X X X X , < F  ...X.X.o.O.F < , ",
	"X X X X Z 6 +.S @.#.$.%.&.*.=.Z ",
	"X X X X ( z -.;.U :.>.;.,.-.z ( ",
	"X X X X <.1.2.2.2.2.2.2.2.2.1.<."
};


/* XPM */
static const char* s_module_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"16 16 38 1",
	"@ c #9BA9B6",
	"> c #F7FAFE",
	"2 c #E45864",
	": c #CDE0F9",
	"O c #8596A5",
	"& c #EBF2FC",
	", c #C1D8F7",
	"% c #FFFFFF",
	"  c #7E92A3",
	"= c #F3F7FD",
	"6 c #CCAFC8",
	"X c #8193A3",
	"0 c #FBFCFE",
	"- c #D1E2F9",
	"7 c #EFF4FC",
	"e c #D78699",
	"4 c #EA424C",
	"3 c #EA424D",
	"w c #F0F5FD",
	"1 c #D88497",
	"8 c #DAE8FA",
	"q c #FDFEFF",
	"$ c #F6F7F8",
	"t c #DD7385",
	"* c #B1CFF5",
	"p c #8E9EAB",
	"9 c #F9FBFE",
	"o c #788B9C",
	"u c #EDF0F2",
	"< c #F7F8FC",
	"y c #E6E9EC",
	"+ c #99A7B4",
	". c #8194A1",
	"# c #90A0AE",
	"r c #DF6C7C",
	"i c #8498A6",
	"5 c #DB788A",
	"; c #EFF5FD",
	/* pixels */
	" .............. ",
	"XooooooooooooooX",
	"O+@@@@@@@@@@@@+O",
	"#$%%%%%%%%%%%%$#",
	"#$%&********&%$#",
	"#$%=--------=%$#",
	"#$%%%%%%%%%%%%$#",
	"#$%;:>,,,,,,;%$#",
	"#$%;:<1234567%$#",
	"#$%=89------=%$#",
	"#$%0=qwwwwww0%$#",
	"#$%0=qwwwwww0%$#",
	"#$%;:>ert,,,;%$#",
	"#$%%%%%%%%%%%%$#",
	"#yuuuuuuuuuuuuy#",
	"ippppppppppppppi"
};

wxIcon IMetaObjectModule::GetIcon() const
{
	return wxIcon(s_module_xpm);
}

wxIcon IMetaObjectModule::GetIconGroup()
{
	return wxIcon(s_module_xpm);
}

wxIcon CMetaObjectCommonModule::GetIcon() const
{
	return wxIcon(s_module_xpm);
}

wxIcon CMetaObjectCommonModule::GetIconGroup()
{
	return wxIcon(s_module_xpm);
}

wxIcon CMetaObjectManagerModule::GetIcon() const
{
	return wxIcon(s_module_xpm);
}

wxIcon CMetaObjectManagerModule::GetIconGroup()
{
	return wxIcon(s_module_xpm);
}
