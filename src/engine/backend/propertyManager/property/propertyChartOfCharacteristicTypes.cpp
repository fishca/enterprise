#include "propertyChartOfCharacteristicTypes.h"
#include "backend/propertyManager/property/variant/variantOwner.h"

wxObject* (*ibPropertyChartOfCharacteristicTypes::ms_propertyChartOfCharacteristicTypes)(ibPropertyObject*, const wxString&, const wxString&, const wxVariant&) = nullptr;

wxVariantData* ibPropertyChartOfCharacteristicTypes::CreateVariantData(ibPropertyObject* property, const ibMetaDescription& typeDesc) const
{
	const ibValueMetaObjectGenericData* propFactory = dynamic_cast<const ibValueMetaObjectGenericData*>(property);
	if (propFactory == nullptr) return nullptr;
	return new ibVariantDataOwner(propFactory, typeDesc);
}

ibMetaDescription& ibPropertyChartOfCharacteristicTypes::GetValueAsMetaDesc() const {
	return get_cell_variant<ibVariantDataOwner>()->GetMetaDesc();
}

void ibPropertyChartOfCharacteristicTypes::SetValue(const ibMetaDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

bool ibPropertyChartOfCharacteristicTypes::SetDataValue(const ibValue& varPropVal) { return false; }

bool ibPropertyChartOfCharacteristicTypes::GetDataValue(ibValue& pvarPropVal) const
{
	const ibVariantDataOwner* owner = get_cell_variant<ibVariantDataOwner>();
	wxASSERT(owner);
	pvarPropVal = owner->GetDataValue();
	return true;
}

bool ibPropertyChartOfCharacteristicTypes::LoadData(ibReaderMemory& reader)
{
	return ibMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool ibPropertyChartOfCharacteristicTypes::SaveData(ibWriterMemory& writer)
{
	return ibMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}
