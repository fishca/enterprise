#include "propertyPicture.h"
#include "backend/propertyManager/property/variant/variantPicture.h"

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyExternalPicture::CreateVariantData(const ibExternalPictureDescription& id) const
{
	return new ibVariantDataExternalPicture(id);
}

////////////////////////////////////////////////////////////////////////

wxBitmap ibPropertyExternalPicture::GetValueAsBitmap() const
{
	return get_cell_variant<ibVariantDataExternalPicture>()->GetPictureBitmap();
}

ibExternalPictureDescription& ibPropertyExternalPicture::GetValueAsPictureDesc() const
{
	return get_cell_variant<ibVariantDataExternalPicture>()->GetExternalPictureDesc();
}

void ibPropertyExternalPicture::SetValue(const ibExternalPictureDescription& val)
{
	m_propValue = CreateVariantData(val);
}

////////////////////////////////////////////////////////////////////////

bool ibPropertyExternalPicture::IsEmptyProperty() const {
	return get_cell_variant<ibVariantDataExternalPicture>()->IsEmptyPicture();
}

#include "backend/system/value/valuePicture.h"

//base property for "external picture"
bool ibPropertyExternalPicture::SetDataValue(const ibValue& varPropVal)
{
	return false;
}

bool ibPropertyExternalPicture::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValuePicture>(GetValueAsPictureDesc());
	return true;
}

bool ibPropertyExternalPicture::LoadData(ibReaderMemory& reader)
{
	return ibExternalPictureDescriptionMemory::LoadData(reader, GetValueAsPictureDesc());
}

bool ibPropertyExternalPicture::SaveData(ibWriterMemory& writer)
{
	return ibExternalPictureDescriptionMemory::SaveData(writer, GetValueAsPictureDesc());
}
