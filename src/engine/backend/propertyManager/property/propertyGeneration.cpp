#include "propertyGeneration.h"
#include "backend/propertyManager/property/variant/variantGen.h"

wxVariantData* CPropertyGeneration::CreateVariantData(IPropertyObject* property, const CMetaDescription& typeDesc) const
{
	const IValueMetaObjectGenericData* propFactory = dynamic_cast<const IValueMetaObjectGenericData*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new wxVariantDataGeneration(propFactory, typeDesc);
}

CMetaDescription& CPropertyGeneration::GetValueAsMetaDesc() const {
	return get_cell_variant<wxVariantDataGeneration>()->GetMetaDesc();
}

void CPropertyGeneration::SetValue(const CMetaDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

//base property for "generation"
bool CPropertyGeneration::SetDataValue(const CValue& varPropVal)
{
	return false;
}

bool CPropertyGeneration::GetDataValue(CValue& pvarPropVal) const
{
	const wxVariantDataGeneration* gen = get_cell_variant<wxVariantDataGeneration>();
	wxASSERT(gen);
	pvarPropVal = gen->GetDataValue();
	return true;
}

bool CPropertyGeneration::LoadData(CMemoryReader& reader)
{
	return CMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool CPropertyGeneration::SaveData(CMemoryWriter& writer)
{
	return CMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}