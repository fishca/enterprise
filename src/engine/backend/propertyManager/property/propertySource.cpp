#include "propertySource.h"
#include "backend/propertyManager/property/variant/sourceVariant.h"

wxVariantData* CPropertySource::CreateVariantData(const IPropertyObject* property, const eValueTypes& type) const
{
    return CreateVariantData(property, CTypeDescription(CValue::GetIDByVT(type)));
}

wxVariantData* CPropertySource::CreateVariantData(const IPropertyObject* property, const class_identifier_t& clsid) const
{
    return CreateVariantData(property, CTypeDescription(clsid));
}

wxVariantData* CPropertySource::CreateVariantData(const IPropertyObject* property, const CTypeDescription& typeDesc) const
{
    const IBackendTypeSourceFactory* propFactory = dynamic_cast<const IBackendTypeSourceFactory*>(property);
    if (propFactory == nullptr)
        return nullptr;
    return new wxVariantDataSource(propFactory, typeDesc);
}

wxVariantData* CPropertySource::CreateVariantData(const IPropertyObject* property, const meta_identifier_t& id) const
{
    const IBackendTypeSourceFactory* propFactory = dynamic_cast<const IBackendTypeSourceFactory*>(property);
    if (propFactory == nullptr)
        return nullptr;
    return new wxVariantDataSource(propFactory, id);
}

wxVariantData* CPropertySource::CreateVariantData(const IPropertyObject* property, const CGuid& id, bool fillTypeDesc) const
{
    const IBackendTypeSourceFactory* propFactory = dynamic_cast<const IBackendTypeSourceFactory*>(property);
    if (propFactory == nullptr)
        return nullptr;
    return new wxVariantDataSource(propFactory, id, fillTypeDesc);
}

////////////////////////////////////////////////////////////////////////
meta_identifier_t CPropertySource::GetValueAsSource() const { return get_cell_variant<wxVariantDataSource>()->GetSource(); }
CGuid CPropertySource::GetValueAsSourceGuid() const { return get_cell_variant<wxVariantDataSource>()->GetSourceGuid(); }
CTypeDescription& CPropertySource::GetValueAsTypeDesc(bool fillTypeDesc) const { return get_cell_variant<wxVariantDataSource>()->GetSourceTypeDesc(fillTypeDesc); }
void CPropertySource::SetValue(const meta_identifier_t& val) { m_propValue = CreateVariantData(m_owner, val); }
void CPropertySource::SetValue(const CGuid& val, bool fillTypeDesc) { m_propValue = CreateVariantData(m_owner, val, fillTypeDesc); }
void CPropertySource::SetValue(const CTypeDescription& val) { m_propValue = CreateVariantData(m_owner, val); }
////////////////////////////////////////////////////////////////////////

IMetaObjectAttribute* CPropertySource::GetSourceAttributeObject() const { 
    return get_cell_variant<wxVariantDataSource>()->GetSourceAttributeObject(); 
}

////////////////////////////////////////////////////////////////////////

bool CPropertySource::IsEmptyProperty() const { 
    return get_cell_variant<wxVariantDataSource>()->IsEmptySource(); 
}

////////////////////////////////////////////////////////////////////////

//base property for "source"
bool CPropertySource::SetDataValue(const CValue& varPropVal)
{
    //varPropVal.GetString();
    return false;
}

bool CPropertySource::GetDataValue(CValue& pvarPropVal) const
{
    pvarPropVal = m_propValue.GetString();
    return true;
}

bool CPropertySource::LoadData(CMemoryReader& reader)
{
    CPropertySource::SetValue(reader.r_stringZ(), false); 
    return CTypeDescriptionMemory::LoadData(reader, GetValueAsTypeDesc(false));
}

bool CPropertySource::SaveData(CMemoryWriter& writer)
{
    writer.w_stringZ(CPropertySource::GetValueAsSourceGuid());
    return CTypeDescriptionMemory::SaveData(writer, GetValueAsTypeDesc());
}