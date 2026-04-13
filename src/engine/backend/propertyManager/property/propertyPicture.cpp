#include "propertyPicture.h"
#include "backend/propertyManager/property/variant/variantPicture.h"

// get property for grid
wxObject* (*ibPropertyPicture::ms_propertyPicture)(const wxString&, const wxString&, const wxVariant&) = nullptr;

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyPicture::CreateVariantData(ibPropertyObject* property, const ibPictureDescription& id) const
{
	return new ibVariantDataPicture(property, id);
}

////////////////////////////////////////////////////////////////////////

wxBitmap ibPropertyPicture::GetValueAsBitmap() const
{
	return get_cell_variant<ibVariantDataPicture>()->GetPictureBitmap();
}

ibPictureDescription& ibPropertyPicture::GetValueAsPictureDesc() const
{
	return get_cell_variant<ibVariantDataPicture>()->GetPictureDesc();
}

void ibPropertyPicture::SetValue(const ibPictureDescription& val)
{
	m_propValue = CreateVariantData(m_owner, val);
}

////////////////////////////////////////////////////////////////////////

bool ibPropertyPicture::IsEmptyProperty() const {
	return get_cell_variant<ibVariantDataPicture>()->IsEmptyPicture();
}

#include "backend/system/value/valuePicture.h"

//base property for "picture"
bool ibPropertyPicture::SetDataValue(const ibValue& varPropVal)
{
	ibValuePicture* valueType = varPropVal.ConvertToType<ibValuePicture>();
	if (valueType != nullptr) {
		SetValue(valueType->GetPictureDesc());
		return true;
	}
	return false;
}

bool ibPropertyPicture::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibValue::CreateObjectValue<ibValuePicture>(GetValueAsPictureDesc());
	return true;
}

bool ibPropertyPicture::LoadData(ibReaderMemory& reader)
{
	return ibPictureDescriptionMemory::LoadData(reader, GetValueAsPictureDesc());
}

bool ibPropertyPicture::SaveData(ibWriterMemory& writer)
{
	return ibPictureDescriptionMemory::SaveData(writer, GetValueAsPictureDesc());
}