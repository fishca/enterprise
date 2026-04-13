////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject menu
////////////////////////////////////////////////////////////////////////////

#include "metaObject.h"
#include "backend/metaData.h"

void ibValueMetaObject::OnPropertyCreated(ibProperty* property)
{
}

void ibValueMetaObject::OnPropertySelected(ibProperty* property)
{
}

bool ibValueMetaObject::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	ibBackendMetadataTree* metadataTree = m_metaData->GetMetaTree();
	if (m_propertyName == property && metadataTree != nullptr)
		return metadataTree->RenameMetaObject(this, newValue.GetString());
	else if (m_propertyName == property)
		return m_metaData->RenameMetaObject(this, newValue.GetString());
	m_metaData->Modify(true);
	return true;
}

#include "backend/backend_mainFrame.h"

void ibValueMetaObject::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyName == property) m_propertySynonym->SetValue(stringUtils::GenerateSynonym(newValue));
	wxASSERT(m_metaData);
	const ibBackendMetadataTree* metadataTree = m_metaData->GetMetaTree();
	if (backend_mainFrame && metadataTree != nullptr) backend_mainFrame->RefreshFrame();
}