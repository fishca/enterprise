#include "variantPicture.h"

wxString wxVariantDataExternalPicture::MakeString() const
{
	if (IsEmptyPicture())
		return wxEmptyString;

	wxFileName fn(m_pictureExternalFile.m_img_name);
	return fn.GetFullName();
}

wxString wxVariantDataExternalPicture::GetPictureFileName() const
{
	return m_pictureExternalFile.m_img_name;
}

#include <wx/mstream.h>

wxBitmap wxVariantDataExternalPicture::GetPictureBitmap(const wxSize& size) const
{
	if (!m_pictureExternalFile.m_img_buffer.empty()) {
		wxMemoryInputStream inputStream(
			m_pictureExternalFile.m_img_buffer.data(),
			m_pictureExternalFile.m_img_buffer.size()
		);
		const wxImage image(inputStream);
		if (size != image.GetSize())
			return image.Scale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
		return image;
	}

	return wxNullBitmap;
}

wxString wxVariantDataPicture::MakeString() const
{
	if (m_pictureDesc.m_type == EPictureType::eFromBackend)
		return _("Backend picture");
	else if (m_pictureDesc.m_type == EPictureType::eFromConfiguration)
		return _("Configuration picture");
	else if (m_pictureDesc.m_type == EPictureType::eFromFile)
		return _("External picture");

	return wxEmptyString;
}

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaPictureObject.h"

wxBitmap wxVariantDataPicture::GetPictureBitmap(const wxSize& size) const
{
	if (m_pictureDesc.m_type == EPictureType::eFromBackend) {
		const IAbstractTypeCtor* so =
			CValue::GetAvailableCtor(m_pictureDesc.m_class_identifier);
		if (so != nullptr) return so->GetClassIcon();
	}
	else if (m_pictureDesc.m_type == EPictureType::eFromConfiguration) {
		if (m_ownerProperty != nullptr) {
			const IMetaData* metaData = m_ownerProperty->GetMetaData();
			if (metaData != nullptr && m_pictureDesc.m_meta_guid.isValid()) {
				CMetaObjectPicture* picture = nullptr;
				if (metaData->GetMetaObject(picture, m_pictureDesc.m_meta_guid))
					return picture->IsAllowed() ? picture->GetValueAsBitmap() : wxNullBitmap;
			}
		}
	}
	else if (m_pictureDesc.m_type == EPictureType::eFromFile) {
		if (!m_pictureDesc.m_img_data.m_img_buffer.empty()) {
			wxMemoryInputStream inputStream(
				m_pictureDesc.m_img_data.m_img_buffer.data(),
				m_pictureDesc.m_img_data.m_img_buffer.size()
			);
			const wxImage image(inputStream);
			if (size != image.GetSize())
				return image.Scale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
			return image;
		}
	}

	return wxNullBitmap;
}

