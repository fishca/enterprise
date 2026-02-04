////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : catalog metaData - menu
////////////////////////////////////////////////////////////////////////////

#include "catalog.h"
#include "backend/metaData.h"

bool CValueMetaObjectCatalog::PrepareContextMenu(wxMenu *defaultMenu)
{
	wxMenuItem *menuItem = nullptr;
	menuItem = defaultMenu->Append(ID_METATREE_OPEN_MODULE, _("Open object module"));
	menuItem->SetBitmap((*m_propertyModuleObject)->GetIcon());
	menuItem = defaultMenu->Append(ID_METATREE_OPEN_MANAGER, _("Open manager module"));
	menuItem->SetBitmap((*m_propertyModuleManager)->GetIcon());
	defaultMenu->AppendSeparator();
	return false;
}

void CValueMetaObjectCatalog::ProcessCommand(unsigned int id)
{
	IBackendMetadataTree *metaTree = m_metaData->GetMetaTree();
	wxASSERT(metaTree);

	if (id == ID_METATREE_OPEN_MODULE)
		metaTree->OpenFormMDI(m_propertyModuleObject->GetMetaObject());
	else if (id == ID_METATREE_OPEN_MANAGER)
		metaTree->OpenFormMDI(m_propertyModuleManager->GetMetaObject());
}