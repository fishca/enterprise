////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : register metaData - menu
////////////////////////////////////////////////////////////////////////////

#include "informationRegister.h"
#include "backend/metaData.h"

bool ibValueMetaObjectInformationRegister::PrepareContextMenu(wxMenu* defaultMenu)
{
	wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_OPEN_MODULE, _("Open record set module"));
	menuItem->SetBitmap((*m_propertyModuleObject)->GetIcon());
	menuItem = defaultMenu->Append(ID_METATREE_OPEN_MANAGER, _("Open manager module"));
	menuItem->SetBitmap((*m_propertyModuleManager)->GetIcon());
	defaultMenu->AppendSeparator();
	return false;
}

void ibValueMetaObjectInformationRegister::ProcessCommand(unsigned int id)
{
	ibBackendMetadataTree* metaTree = m_metaData->GetMetaTree();
	wxASSERT(metaTree);

	if (id == ID_METATREE_OPEN_MODULE)
		metaTree->OpenFormMDI(m_propertyModuleObject->GetMetaObject());
	else if (id == ID_METATREE_OPEN_MANAGER)
		metaTree->OpenFormMDI(m_propertyModuleManager->GetMetaObject());
}