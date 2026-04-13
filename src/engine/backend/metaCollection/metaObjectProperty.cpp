////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject menu
////////////////////////////////////////////////////////////////////////////

#include "metaObject.h"
#include "backend/metaData.h"

void IValueMetaObject::OnPropertyCreated(IProperty* property)
{
}

void IValueMetaObject::OnPropertySelected(IProperty* property)
{
}

bool IValueMetaObject::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	IBackendMetadataTree* metadataTree = m_metaData->GetMetaTree();
	if (m_propertyName == property && metadataTree != nullptr)
		return metadataTree->RenameMetaObject(this, newValue.GetString());
	else if (m_propertyName == property)
		return m_metaData->RenameMetaObject(this, newValue.GetString());
	m_metaData->Modify(true);
	return true;
}

#include "backend/backend_mainFrame.h"

void IValueMetaObject::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (m_propertyName == property) m_propertySynonym->SetValue(stringUtils::GenerateSynonym(newValue));
	wxASSERT(m_metaData);
	const IBackendMetadataTree* metadataTree = m_metaData->GetMetaTree();
	if (backend_mainFrame && metadataTree != nullptr) backend_mainFrame->RefreshFrame();
}