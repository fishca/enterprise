#include "pictureDescription.h"
#include "backend/fileSystem/fs.h"

#define chunkNamePicture 0x323456534
#define chunkBufferPicture 0x323456535

////////////////////////////////////////////////////////////////////////

bool ibPictureDescriptionMemory::LoadData(ibReaderMemory& reader, ibPictureDescription& pictureDesc)
{
	pictureDesc.m_type = static_cast<ibPictureType>(reader.r_u32());

	if (pictureDesc.m_type == ibPictureType::eFromBackend) {
		pictureDesc.m_class_identifier = reader.r_u64();
	}
	else if (pictureDesc.m_type == ibPictureType::eFromConfiguration) {
		pictureDesc.m_meta_guid = reader.r_stringZ();
	}
	else if (pictureDesc.m_type == ibPictureType::eFromFile) {
		ibExternalPictureDescriptionMemory::LoadData(reader, pictureDesc.m_img_data);
	}

	return true;
}

bool ibExternalPictureDescriptionMemory::LoadData(ibReaderMemory& reader, ibExternalPictureDescription& pictureDesc)
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

bool ibPictureDescriptionMemory::SaveData(ibWriterMemory& writer, ibPictureDescription& pictureDesc)
{
	writer.w_u32(pictureDesc.m_type);

	if (pictureDesc.m_type == ibPictureType::eFromBackend) {
		writer.w_u64(pictureDesc.m_class_identifier);
	}
	else if (pictureDesc.m_type == ibPictureType::eFromConfiguration) {
		writer.w_stringZ(pictureDesc.m_meta_guid);
	}
	else if (pictureDesc.m_type == ibPictureType::eFromFile) {
		ibExternalPictureDescriptionMemory::SaveData(writer, pictureDesc.m_img_data);
	}

	return true;
}

bool ibExternalPictureDescriptionMemory::SaveData(ibWriterMemory& writer, ibExternalPictureDescription& pictureDesc)
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