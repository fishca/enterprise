#include "propertyOwner.h"
#include "backend/propertyManager/property/variant/variantOwner.h"

wxObject* (*ibPropertyOwner::ms_propertyOwner)(ibPropertyObject*, const wxString&, const wxString&, const wxVariant&) = nullptr;

////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyOwner::CreateVariantData(ibPropertyObject* property, const ibMetaDescription& typeDesc) const
{
	const ibValueMetaObjectGenericData* propFactory = dynamic_cast<const ibValueMetaObjectGenericData*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new ibVariantDataOwner(propFactory, typeDesc);
}

ibMetaDescription& ibPropertyOwner::GetValueAsMetaDesc() const {
	return get_cell_variant<ibVariantDataOwner>()->GetMetaDesc();
}

void ibPropertyOwner::SetValue(const ibMetaDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

//base property for "owner"
bool ibPropertyOwner::SetDataValue(const ibValue& varPropVal)
{
	return false;
}

bool ibPropertyOwner::GetDataValue(ibValue& pvarPropVal) const
{
	const ibVariantDataOwner* owner = get_cell_variant<ibVariantDataOwner>();
	wxASSERT(owner);
	pvarPropVal = owner->GetDataValue();
	return true;
}

bool ibPropertyOwner::LoadData(ibReaderMemory& reader)
{
	return ibMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool ibPropertyOwner::SaveData(ibWriterMemory& writer)
{
	return ibMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}
