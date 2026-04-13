#include "toolbar.h"

enum
{
	MENU_ADDITEM = 1000,
	MENU_ADDITEM_SEPARATOR,
};

void ibValueToolbar::PrepareDefaultMenu(wxMenu* menu)
{
	menu->Append(MENU_ADDITEM, _("Add tool\tInsert"))->SetBitmap(ibValueToolBarItem::GetIconGroup());
	menu->Append(MENU_ADDITEM_SEPARATOR, _("Add separator\tInsert"))->SetBitmap(ibValueToolBarSeparator::GetIconGroup());
	menu->AppendSeparator();
}

void ibValueToolbar::ExecuteMenu(ibVisualHost* visualHost, int id)
{
	switch (id)
	{
	case MENU_ADDITEM:
		AddToolItem();
		break;
	case MENU_ADDITEM_SEPARATOR:
		AddToolSeparator();
		break;
	}
}