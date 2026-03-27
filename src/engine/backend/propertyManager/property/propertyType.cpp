#include "propertyType.h"
#include "backend/propertyManager/property/variant/variantType.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyType::CreateVariantData(ibPropertyObject* property, const ibValueTypes type) const
{
	const ibBackendTypeConfigFactory* propFactory = dynamic_cast<const ibBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new ibVariantDataAttribute(propFactory, type);
}

wxVariantData* ibPropertyType::CreateVariantData(ibPropertyObject* property, const ibClassID& clsid) const
{
	const ibBackendTypeConfigFactory* propFactory = dynamic_cast<const ibBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new ibVariantDataAttribute(propFactory, clsid);
}

wxVariantData* ibPropertyType::CreateVariantData(ibPropertyObject* property, const ibTypeDescription& typeDesc) const
{
	const ibBackendTypeConfigFactory* propFactory = dynamic_cast<const ibBackendTypeConfigFactory*>(property);
	if (propFactory == nullptr)
		return nullptr;
	return new ibVariantDataAttribute(propFactory, typeDesc);
}

////////////////////////////////////////////////////////////////////////

ibTypeDescription& ibPropertyType::GetValueAsTypeDesc() const {
	return get_cell_variant<ibVariantDataAttribute>()->GetTypeDesc();
}

void ibPropertyType::SetValue(const ibTypeDescription& val) {
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

ibSelectorDataType ibPropertyType::GetFilterDataType() const {
	return get_cell_variant<ibVariantDataAttribute>()->GetFilterDataType();
}

////////////////////////////////////////////////////////////////////////

#include "backend/system/value/valueType.h"

//base property for "type"
bool ibPropertyType::SetDataValue(const ibValue& varPropVal)
{
	ibValueTypeDescription* valueType = varPropVal.ConvertToType<ibValueTypeDescription>();
	if (valueType != nullptr) {
		SetValue(valueType->GetTypeDesc());
		return true;
	}
	return false;
}

bool ibPropertyType::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValueTypeDescription>(GetValueAsTypeDesc());
	return true;
}

bool ibPropertyType::LoadData(ibReaderMemory& reader)
{
	return ibTypeDescriptionMemory::LoadData(reader, GetValueAsTypeDesc());
}

bool ibPropertyType::SaveData(ibWriterMemory& writer)
{
	return ibTypeDescriptionMemory::SaveData(writer, GetValueAsTypeDesc());
}
