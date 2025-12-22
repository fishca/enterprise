#ifndef __IMAGE_HELPER_H__
#define __IMAGE_HELPER_H__

#include "backend/compiler/value.h"

enum EPictureType
{
	eFromBackend = 1,
	eFromConfiguration,
	eFromFile,
};

struct CExternalPictureDescription {

	bool IsEmptyPicture() const { return m_height == 0 && m_width == 0; }

	bool operator ==(const CExternalPictureDescription& rhs) const {
		return m_img_buffer == rhs.m_img_buffer
			&& m_width == rhs.m_width
			&& m_height == rhs.m_height;

		return false;
	}

	std::string m_img_name, m_img_buffer;
	unsigned int m_width = 0, m_height = 0;
};

class CExternalPictureDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CExternalPictureDescription& pictureDesc);
	static bool SaveData(class CMemoryWriter& writer, CExternalPictureDescription& pictureDesc);
};

struct CPictureDescription {

	EPictureType m_type;

	CPictureDescription() : m_type(eFromBackend) {}
	CPictureDescription(const picture_identifier_t& id) : m_type(eFromBackend), m_class_identifier(id) {}
	CPictureDescription(const CGuid& id) : m_type(eFromConfiguration), m_meta_guid(id) {}
	CPictureDescription(const CExternalPictureDescription& data) : m_type(eFromFile), m_img_data(data) {}
	
	~CPictureDescription() {}

	bool IsEmptyPicture() const {
		if (m_type == eFromBackend)
			return m_class_identifier == 0;
		else if (m_type == eFromConfiguration)
			return !m_meta_guid.isValid();
		else if (m_type == eFromFile)
			return m_img_data.m_height == 0 && m_img_data.m_width == 0;
		return true;
	}

	bool operator ==(const CPictureDescription& rhs) const
	{
		if (m_type == rhs.m_type) {
			if (m_type == eFromBackend) {
				return m_class_identifier == rhs.m_class_identifier;
			}
			else if (m_type == eFromConfiguration) {
				return m_meta_guid == rhs.m_meta_guid;
			}
			else if (m_type == eFromFile) {
				return m_img_data.m_img_buffer == rhs.m_img_data.m_img_buffer
					&& m_img_data.m_width == rhs.m_img_data.m_width
					&& m_img_data.m_height == rhs.m_img_data.m_height;
			}
		}

		return false;
	}

	picture_identifier_t m_class_identifier = 0;
	CGuid m_meta_guid;
	CExternalPictureDescription m_img_data;
};

class CPictureDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CPictureDescription& pictureDesc);
	static bool SaveData(class CMemoryWriter& writer, CPictureDescription& pictureDesc);
};

extern BACKEND_API bool RegisterBackendPicture(const wxString name,
	const picture_identifier_t& id, const wxBitmap& image);

#endif 
