////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataProcessor - menu
////////////////////////////////////////////////////////////////////////////

#include "dataProcessor.h"
#include "backend/metaData.h"

bool CMetaObjectDataProcessor::PrepareContextMenu(wxMenu *defaultMenu)
{
	wxMenuItem *menuItem = defaultMenu->Append(ID_METATREE_OPEN_MODULE, _("Open object module"));
	menuItem->SetBitmap((*m_propertyModuleObject)->GetIcon());
	menuItem = defaultMenu->Append(ID_METATREE_OPEN_MANAGER, _("Open manager module"));
	menuItem->SetBitmap((*m_propertyModuleManager)->GetIcon());
	defaultMenu->AppendSeparator();
	return false;
}

void CMetaObjectDataProcessor::ProcessCommand(unsigned int id)
{
	IBackendMetadataTree *metaTree = m_metaData->GetMetaTree();
	wxASSERT(metaTree);

	if (id == ID_METATREE_OPEN_MODULE)
		metaTree->OpenFormMDI(m_propertyModuleObject->GetMetaObject());
	else if (id == ID_METATREE_OPEN_MANAGER)
		metaTree->OpenFormMDI(m_propertyModuleManager->GetMetaObject());
}