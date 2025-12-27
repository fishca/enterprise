#include "toolbar.h"

enum
{
	MENU_ADDITEM = 1000,
	MENU_ADDITEM_SEPARATOR,
};

void CValueToolbar::PrepareDefaultMenu(wxMenu* menu)
{
	menu->Append(MENU_ADDITEM, _("Add tool\tInsert"))->SetBitmap(CValueToolBarItem::GetIconGroup());
	menu->Append(MENU_ADDITEM_SEPARATOR, _("Add separator\tInsert"))->SetBitmap(CValueToolBarSeparator::GetIconGroup());
	menu->AppendSeparator();
}

void CValueToolbar::ExecuteMenu(IVisualHost* visualHost, int id)
{
	switch (id)
	{
	case MENU_ADDITEM:
		this->AddToolItem();
		break;
	case MENU_ADDITEM_SEPARATOR:
		this->AddToolSeparator();
		break;
	}
}