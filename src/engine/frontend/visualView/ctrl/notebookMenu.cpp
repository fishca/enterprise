#include "notebook.h"

enum
{
	MENU_ADDPAGE = 1000
};

void CValueNotebook::PrepareDefaultMenu(wxMenu* menu)
{
	menu->Append(MENU_ADDPAGE, wxT("Add page\tInsert"))->SetBitmap(CValueNotebookPage::GetIconGroup());
	menu->AppendSeparator();
}

void CValueNotebook::ExecuteMenu(IVisualHost* visualHost, int id)
{
	if (id == MENU_ADDPAGE) AddNotebookPage();
}
