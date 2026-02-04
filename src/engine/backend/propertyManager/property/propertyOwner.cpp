#include "propertyOwner.h"
#include "backend/propertyManager/property/variant/variantOwner.h"

wxVariantData* CPropertyOwner::CreateVariantData(IPropertyObject* property, const CMetaDescription& typeDesc) const
{
    const IValueMetaObjectGenericData* propFactory = dynamic_cast<const IValueMetaObjectGenericData*>(property);
    if (propFactory == nullptr)
        return nullptr;
    return new wxVariantDataOwner(propFactory, typeDesc);
}

CMetaDescription& CPropertyOwner::GetValueAsMetaDesc() const {
    return get_cell_variant<wxVariantDataOwner>()->GetMetaDesc();
}

void CPropertyOwner::SetValue(const CMetaDescription& val)
{
    m_propValue = CreateVariantData(m_owner, val);
}

//base property for "owner"
bool CPropertyOwner::SetDataValue(const CValue& varPropVal)
{
    return false;
}

bool CPropertyOwner::GetDataValue(CValue& pvarPropVal) const
{
    const wxVariantDataOwner* owner = get_cell_variant<wxVariantDataOwner>();
    wxASSERT(owner);  
    pvarPropVal = owner->GetDataValue();
    return true;
}

bool CPropertyOwner::LoadData(CMemoryReader& reader)
{
    return CMetaDescriptionMemory::LoadData(reader, GetValueAsMetaDesc());
}

bool CPropertyOwner::SaveData(CMemoryWriter& writer)
{
    return CMetaDescriptionMemory::SaveData(writer, GetValueAsMetaDesc());
}
