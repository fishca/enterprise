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

wxBitmap wxVariantDataExternalPicture::GetPictureBitmap(const wxSize& size) const
{
	return CBackendPicture::CreatePicture(m_pictureExternalFile, size);
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

wxBitmap wxVariantDataPicture::GetPictureBitmap(const wxSize& size) const
{
	return CBackendPicture::CreatePicture(m_pictureDesc,
		m_ownerProperty != nullptr ? m_ownerProperty->GetMetaData() : nullptr, size);
}

