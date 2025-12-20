#include "pictureDescription.h"
#include "backend/fileSystem/fs.h"

#define chunkNamePicture 0x323456534
#define chunkBufferPicture 0x323456535

////////////////////////////////////////////////////////////////////////

bool CPictureDescriptionMemory::LoadData(CMemoryReader& reader, CPictureDescription& pictureDesc)
{
	pictureDesc.m_type = static_cast<EPictureType>(reader.r_u32());

	if (pictureDesc.m_type == EPictureType::eFromBackend) {
		pictureDesc.m_class_identifier = reader.r_u64();
	}
	else if (pictureDesc.m_type == EPictureType::eFromConfiguration) {
		pictureDesc.m_meta_guid = reader.r_stringZ();
	}
	else if (pictureDesc.m_type == EPictureType::eFromFile) {
		CExternalPictureDescriptionMemory::LoadData(reader, pictureDesc.m_img_data);
	}

	return true;
}

bool CExternalPictureDescriptionMemory::LoadData(CMemoryReader& reader, CExternalPictureDescription& pictureDesc)
{
	wxMemoryBuffer namePicture, bufferPicture;

	if (reader.r_chunk(chunkNamePicture, namePicture)) {
		pictureDesc.m_img_name.assign(
			(const char*)namePicture.GetData(),
			namePicture.GetDataLen()
		);
	}

	if (reader.r_chunk(chunkBufferPicture, bufferPicture)) {
		pictureDesc.m_img_buffer.assign(
			(const char*)bufferPicture.GetData(),
			bufferPicture.GetDataLen()
		);
	}

	pictureDesc.m_height = reader.r_u32();
	pictureDesc.m_width = reader.r_u32();
	return true;
}

bool CPictureDescriptionMemory::SaveData(CMemoryWriter& writer, CPictureDescription& pictureDesc)
{
	writer.w_u32(pictureDesc.m_type);

	if (pictureDesc.m_type == EPictureType::eFromBackend) {
		writer.w_u64(pictureDesc.m_class_identifier);
	}
	else if (pictureDesc.m_type == EPictureType::eFromConfiguration) {
		writer.w_stringZ(pictureDesc.m_meta_guid);
	}
	else if (pictureDesc.m_type == EPictureType::eFromFile) {
		CExternalPictureDescriptionMemory::SaveData(writer, pictureDesc.m_img_data);
	}

	return true;
}

bool CExternalPictureDescriptionMemory::SaveData(CMemoryWriter& writer, CExternalPictureDescription& pictureDesc)
{
	wxMemoryBuffer namePicture, bufferPicture;

	namePicture.AppendData(
		pictureDesc.m_img_name.c_str(),
		pictureDesc.m_img_name.size()
	);

	bufferPicture.AppendData(
		pictureDesc.m_img_buffer.c_str(),
		pictureDesc.m_img_buffer.size()
	);

	writer.w_chunk(chunkNamePicture, namePicture);
	writer.w_chunk(chunkBufferPicture, bufferPicture);
	writer.w_u32(pictureDesc.m_height);
	writer.w_u32(pictureDesc.m_width);

	return true;
}

////////////////////////////////////////////////////////////////////////

bool CExternalPictureDescription::IsEmptyPicture() const
{
	return m_height == 0 && m_width == 0;
}

bool CPictureDescription::IsEmptyPicture() const
{
	if (m_type == eFromBackend)
		return m_class_identifier != 0 && CValue::IsRegisterCtor(m_class_identifier);
	else if (m_type == eFromConfiguration)
		return !m_meta_guid.isValid();
	else if (m_type == eFromFile)
		return m_img_data.m_height == 0 && m_img_data.m_width == 0;
	return false;
}
