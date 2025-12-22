#include "backend_picture.h"

#include <wx/mstream.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::vector<CBackendPictureEntry> s_arrayPicture;

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RegisterBackendPicture(const wxString name, const picture_identifier_t& id, const wxBitmap& image)
{
	if (!CBackendPicture::IsRegisterPicture(id)) {
		CBackendPicture::AppendPicture(name, id, image);
		return true;
	}

	wxASSERT_MSG(false, "Picture is register");
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

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
		return CBackendPicture::GetPicture(pictureDesc.m_class_identifier);
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

#pragma region __picture_factory_h__
bool CBackendPicture::IsRegisterPicture(const picture_identifier_t& id)
{
	auto iterator = std::find_if(s_arrayPicture.begin(), s_arrayPicture.end(),
		[id](const auto entry) { return entry.m_id == id; });

	if (iterator != s_arrayPicture.end())
		return true;

	const IAbstractTypeCtor* so = CValue::GetAvailableCtor(id);
	if (so != nullptr && (so->GetObjectTypeCtor() == eCtorObjectType_object_control || so->GetObjectTypeCtor() == eCtorObjectType_object_metadata))
		return true;

	return false;
}

void CBackendPicture::AppendPicture(const wxString name, const picture_identifier_t& id, const wxBitmap& image)
{
	CBackendPictureEntry entry;
	entry.m_name = name;
	entry.m_id = id;
	entry.m_image = image;
	s_arrayPicture.push_back(entry);
}

wxBitmap CBackendPicture::GetPicture(const picture_identifier_t& id)
{
	auto iterator = std::find_if(s_arrayPicture.begin(), s_arrayPicture.end(),
		[id](const auto entry) { return entry.m_id == id; });

	if (iterator != s_arrayPicture.end())
		return iterator->m_image;

	const IAbstractTypeCtor* so = CValue::GetAvailableCtor(id);
	if (so != nullptr)
		return so->GetClassIcon();

	return wxNullBitmap;
}

std::vector<CBackendPictureEntry> CBackendPicture::GetArrayPicture()
{
	std::vector<CBackendPictureEntry> arrayPicture = { s_arrayPicture };

	for (auto so : CValue::GetListCtorsByType(eCtorObjectType::eCtorObjectType_object_metadata)) {
		const wxIcon backend_icon = so->GetClassIcon();
		if (backend_icon.IsOk()) {
			CBackendPictureEntry entry;
			entry.m_name = so->GetClassName();
			entry.m_id = so->GetClassType();
			entry.m_image = backend_icon;
			arrayPicture.push_back(entry);
		}
	}

	for (auto so : CValue::GetListCtorsByType(eCtorObjectType::eCtorObjectType_object_control)) {
		const wxIcon backend_icon = so->GetClassIcon();
		if (backend_icon.IsOk()) {
			CBackendPictureEntry entry;
			entry.m_name = so->GetClassName();
			entry.m_id = so->GetClassType();
			entry.m_image = backend_icon;
			arrayPicture.push_back(entry);
		}
	}

	return arrayPicture;
}
#pragma endregion
