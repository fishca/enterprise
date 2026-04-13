#include "propertyPicture.h"
#include "backend/propertyManager/property/variant/variantPicture.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertyPicture::CreateVariantData(IPropertyObject* property, const CPictureDescription& id) const
{
	return new wxVariantDataPicture(property, id);
}

////////////////////////////////////////////////////////////////////////

wxBitmap CPropertyPicture::GetValueAsBitmap() const
{
	return get_cell_variant<wxVariantDataPicture>()->GetPictureBitmap();
}

CPictureDescription& CPropertyPicture::GetValueAsPictureDesc() const
{
	return get_cell_variant<wxVariantDataPicture>()->GetPictureDesc();
}

void CPropertyPicture::SetValue(const CPictureDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

bool CPropertyPicture::IsEmptyProperty() const {
	return get_cell_variant<wxVariantDataPicture>()->IsEmptyPicture();
}

#include "backend/system/value/valuePicture.h"

//base property for "picture"
bool CPropertyPicture::SetDataValue(const CValue& varPropVal)
{
	CValuePicture* valueType = varPropVal.ConvertToType<CValuePicture>();
	if (valueType != nullptr) {
		SetValue(valueType->GetPictureDesc());
		return true;
	}
	return false;
}

bool CPropertyPicture::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValuePicture>(GetValueAsPictureDesc());
	return true;
}

bool CPropertyPicture::LoadData(CMemoryReader& reader)
{
	return CPictureDescriptionMemory::LoadData(reader, GetValueAsPictureDesc());
}

bool CPropertyPicture::SaveData(CMemoryWriter& writer)
{
	return CPictureDescriptionMemory::SaveData(writer, GetValueAsPictureDesc());
}
