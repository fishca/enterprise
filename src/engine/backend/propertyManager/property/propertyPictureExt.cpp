#include "propertyPicture.h"
#include "backend/propertyManager/property/variant/variantPicture.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertyExternalPicture::CreateVariantData(const CExternalPictureDescription& id) const
{
	return new wxVariantDataExternalPicture(id);
}

////////////////////////////////////////////////////////////////////////

wxBitmap CPropertyExternalPicture::GetValueAsBitmap() const
{
	return get_cell_variant<wxVariantDataExternalPicture>()->GetPictureBitmap();
}

CExternalPictureDescription& CPropertyExternalPicture::GetValueAsPictureDesc() const
{
	return get_cell_variant<wxVariantDataExternalPicture>()->GetExternalPictureDesc();
}

void CPropertyExternalPicture::SetValue(const CExternalPictureDescription& val)
{
	m_propValue = CreateVariantData(val);
}

////////////////////////////////////////////////////////////////////////

bool CPropertyExternalPicture::IsEmptyProperty() const {
	return get_cell_variant<wxVariantDataExternalPicture>()->IsEmptyPicture();
}

#include "backend/system/value/valuePicture.h"

//base property for "external picture"
bool CPropertyExternalPicture::SetDataValue(const CValue& varPropVal)
{
	return false;
}

bool CPropertyExternalPicture::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CValue::CreateObjectValue<CValuePicture>(GetValueAsPictureDesc());
	return true;
}

bool CPropertyExternalPicture::LoadData(CMemoryReader& reader)
{
	return CExternalPictureDescriptionMemory::LoadData(reader, GetValueAsPictureDesc());
}

bool CPropertyExternalPicture::SaveData(CMemoryWriter& writer)
{
	return CExternalPictureDescriptionMemory::SaveData(writer, GetValueAsPictureDesc());
}
