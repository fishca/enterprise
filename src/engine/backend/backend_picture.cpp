#include "backend_picture.h"

#include <wx/mstream.h>

bool CBackendPicture::LoadFromFile(const wxString& strFileName, CExternalPictureDescription& pictureDesc)
{
	wxFileName filename = strFileName;

	// Cache the image
	if (filename.FileExists()) {

		wxImage image(filename.GetFullPath());
		wxMemoryOutputStream outputStream;

		if (image.SaveFile(outputStream, wxBitmapType::wxBITMAP_TYPE_PNG)) {

			// Get the data from the output stream's buffer
			// The output stream manages the memory buffer internally.
			const size_t buffer_len = outputStream.GetLength();

			pictureDesc.m_img_name = filename.GetFullPath();
			pictureDesc.m_img_buffer.resize(buffer_len, '\0');

			outputStream.CopyTo(pictureDesc.m_img_buffer.data(), buffer_len);

			pictureDesc.m_width = image.GetWidth();
			pictureDesc.m_height = image.GetHeight();
			return true;
		}
	}

	return false;
}

bool CBackendPicture::LoadFromFile(const wxString& strFileName, CPictureDescription& pictureDesc)
{
	if (CBackendPicture::LoadFromFile(strFileName, pictureDesc.m_img_data)) {
		pictureDesc.m_type = EPictureType::eFromFile;
		return true;
	}

	return false;
}

wxBitmap CBackendPicture::CreatePicture(const CExternalPictureDescription& pictureDesc, const wxSize& size)
{
	if (!pictureDesc.m_img_buffer.empty()) {
		wxMemoryInputStream inputStream(
			pictureDesc.m_img_buffer.data(),
			pictureDesc.m_img_buffer.size()
		);
		const wxImage image(inputStream);
		if (size != image.GetSize())
			return image.Scale(size.x, size.y, wxIMAGE_QUALITY_HIGH);
		return image;
	}

	return wxNullBitmap;
}

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaPictureObject.h"

wxBitmap CBackendPicture::CreatePicture(const CPictureDescription& pictureDesc, const IMetaData* metaData, const wxSize& size)
{
	if (pictureDesc.m_type == EPictureType::eFromBackend) {
		const IAbstractTypeCtor* so =
			CValue::GetAvailableCtor(pictureDesc.m_class_identifier);
		if (so != nullptr) return so->GetClassIcon();
	}
	else if (pictureDesc.m_type == EPictureType::eFromConfiguration) {
		if (metaData != nullptr && pictureDesc.m_meta_guid.isValid()) {
			CMetaObjectPicture* picture = nullptr;
			if (metaData->GetMetaObject(picture, pictureDesc.m_meta_guid))
				return picture->IsAllowed() ? picture->GetValueAsBitmap() : wxNullBitmap;
		}
	}
	else if (pictureDesc.m_type == EPictureType::eFromFile) {
		return CBackendPicture::CreatePicture(pictureDesc.m_img_data, size);
	}

	return wxNullBitmap;
}