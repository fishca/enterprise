////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject common metaData - menu
////////////////////////////////////////////////////////////////////////////

#include "metaObjectMetadata.h"
#include "metaModuleObject.h"
#include "backend/metaData.h"

bool CMetaObjectConfiguration::PrepareContextMenu(wxMenu *defaultMenu)
{
	wxMenuItem *menuItem = defaultMenu->Append(ID_METATREE_OPEN_INIT_MODULE, _("Open configuration module"));
	menuItem->SetBitmap((*m_propertyModuleConfiguration)->GetIcon());
	return true;
}

void CMetaObjectConfiguration::ProcessCommand(unsigned int id)
{
	IBackendMetadataTree *metaTree = m_metaData->GetMetaTree();
	wxASSERT(metaTree);

	if (id == ID_METATREE_OPEN_INIT_MODULE)
		metaTree->OpenFormMDI(m_propertyModuleConfiguration->GetMetaObject());
}