#include "notebook.h"

enum
{
	MENU_ADDPAGE = 1000
};

void ibValueNotebook::PrepareDefaultMenu(wxMenu* menu)
{
	menu->Append(MENU_ADDPAGE, _("Add page\tInsert"))->SetBitmap(ibValueNotebookPage::GetIconGroup());
	menu->AppendSeparator();
}

void ibValueNotebook::ExecuteMenu(ibVisualHost* visualHost, int id)
{
	if (id == MENU_ADDPAGE) AddNotebookPage();
}
