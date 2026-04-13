#include "tableBox.h"

enum
{
	MENU_ADD_COLUMN = 1000
};

void ibValueModelTableBox::PrepareDefaultMenu(wxMenu *menu)
{
	menu->Append(MENU_ADD_COLUMN, _("Add column\tInsert"))->SetBitmap(ibValueModelTableBoxColumn::GetIconGroup());
	menu->AppendSeparator();
}

void ibValueModelTableBox::ExecuteMenu(ibVisualHost *visualHost, int id)
{
	switch (id)
	{
	case MENU_ADD_COLUMN: 
		AddColumn();
		break;
	}
}