#include "propertyType.h"
#include "backend/propertyManager/property/variant/variantType.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertyType::CreateVariantData(IPropertyObject* property, const eValueTypes type) const
{
	const IBackendTypeConfigFactory* propFactory = dynamic_cast<const IBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new wxVariantDataAttribute(propFactory, type);
}

wxVariantData* CPropertyType::CreateVariantData(IPropertyObject* property, const class_identifier_t& clsid) const
{
	const IBackendTypeConfigFactory* propFactory = dynamic_cast<const IBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new wxVariantDataAttribute(propFactory, clsid);
}

wxVariantData* CPropertyType::CreateVariantData(IPropertyObject* property, const CTypeDescription& typeDesc) const
{
	const IBackendTypeConfigFactory* propFactory = dynamic_cast<const IBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new wxVariantDataAttribute(propFactory, typeDesc);
}

////////////////////////////////////////////////////////////////////////

CTypeDescription& CPropertyType::GetValueAsTypeDesc() const {
	return get_cell_variant<wxVariantDataAttribute>()->GetTypeDesc();
}

void CPropertyType::SetValue(const CTypeDescription& val) {
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

eSelectorDataType CPropertyType::GetFilterDataType() const {
	return get_cell_variant<wxVariantDataAttribute>()->GetFilterDataType();
}

////////////////////////////////////////////////////////////////////////

#include "backend/system/value/valueType.h"

//base property for "type"
bool CPropertyType::SetDataValue(const CValue& varPropVal)
{
	CValueTypeDescription* valueType = varPropVal.ConvertToType<CValueTypeDescription>();
	if (valueType != nullptr) {
		SetValue(valueType->GetTypeDesc());
		return true;
	}
	return false;
}

bool CPropertyType::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValueTypeDescription>(GetValueAsTypeDesc());
	return true;
}

bool CPropertyType::LoadData(CMemoryReader& reader)
{
	return CTypeDescriptionMemory::LoadData(reader, GetValueAsTypeDesc());
}

bool CPropertyType::SaveData(CMemoryWriter& writer)
{
	return CTypeDescriptionMemory::SaveData(writer, GetValueAsTypeDesc());
}
