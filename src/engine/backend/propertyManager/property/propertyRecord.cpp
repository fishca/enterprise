#include "propertyRecord.h"
#include "backend/propertyManager/property/variant/variantRecord.h"

wxVariantData* CPropertyRecord::CreateVariantData(IPropertyObject* property, const CMetaDescription& typeDesc) const
{
	const IMetaObjectGenericData* propFactory = dynamic_cast<const IMetaObjectGenericData*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new wxVariantDataRecord(propFactory, typeDesc);
}

CMetaDescription& CPropertyRecord::GetValueAsMetaDesc() const {
	return get_cell_variant<wxVariantDataRecord>()->GetMetaDesc();
}

CMetaDescription& CPropertyRecord::GetValueAsMetaDesc(const wxVariant& val) const {
	return get_cell_variant<wxVariantDataRecord>(val)->GetMetaDesc();
}

void CPropertyRecord::SetValue(const CMetaDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

//base property for "record"
bool CPropertyRecord::SetDataValue(const CValue& varPropVal)
{
	return false;
}

bool CPropertyRecord::GetDataValue(CValue& pvarPropVal) const
{
	const wxVariantDataRecord* gen = get_cell_variant<wxVariantDataRecord>();
	wxASSERT(gen);
	pvarPropVal = gen->GetDataValue();
	return true;
}

bool CPropertyRecord::LoadData(CMemoryReader& reader)
{
	return CMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool CPropertyRecord::SaveData(CMemoryWriter& writer)
{
	return CMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}