////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enumeration metaData - menu
////////////////////////////////////////////////////////////////////////////

#include "enumeration.h"
#include "backend/metaData.h"

bool CMetaObjectEnumeration::PrepareContextMenu(wxMenu *defaultMenu)
{
	wxMenuItem *menuItem = defaultMenu->Append(ID_METATREE_OPEN_MANAGER, _("Open manager module"));
	menuItem->SetBitmap((*m_propertyModuleManager)->GetIcon());
	defaultMenu->AppendSeparator();
	return false;
}

void CMetaObjectEnumeration::ProcessCommand(unsigned int id)
{
	IBackendMetadataTree *metaTree = m_metaData->GetMetaTree();
	wxASSERT(metaTree);

	if (id == ID_METATREE_OPEN_MANAGER)
		metaTree->OpenFormMDI(m_propertyModuleManager->GetMetaObject());
}