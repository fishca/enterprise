#include "propertyGeneration.h"
#include "backend/propertyManager/property/variant/variantGen.h"

wxVariantData* ibPropertyGeneration::CreateVariantData(ibPropertyObject* property, const ibMetaDescription& typeDesc) const
{
	const ibValueMetaObjectGenericData* propFactory = dynamic_cast<const ibValueMetaObjectGenericData*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new ibVariantDataGeneration(propFactory, typeDesc);
}

ibMetaDescription& ibPropertyGeneration::GetValueAsMetaDesc() const {
	return get_cell_variant<ibVariantDataGeneration>()->GetMetaDesc();
}

void ibPropertyGeneration::SetValue(const ibMetaDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

//base property for "generation"
bool ibPropertyGeneration::SetDataValue(const ibValue& varPropVal)
{
	return false;
}

bool ibPropertyGeneration::GetDataValue(ibValue& pvarPropVal) const
{
	const ibVariantDataGeneration* gen = get_cell_variant<ibVariantDataGeneration>();
	wxASSERT(gen);
	pvarPropVal = gen->GetDataValue();
	return true;
}

bool ibPropertyGeneration::LoadData(ibReaderMemory& reader)
{
	return ibMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool ibPropertyGeneration::SaveData(ibWriterMemory& writer)
{
	return ibMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}