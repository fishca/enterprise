#ifndef __BACKEND_PICTURE_H__
#define __BACKEND_PICTURE_H__

#include "pictureDescription.h"
#include "picturePredefined.h"

struct CBackendPictureEntry {
	wxString m_name;
	wxBitmap m_data;
	picture_identifier_t m_id;
};

class BACKEND_API CBackendPicture {
	CBackendPicture() = delete;
public:

	static void RegisterPicture(const wxString name, const picture_identifier_t& id, const wxBitmap& bitmap);

	static bool LoadFromFile(const wxString& strFileName, CExternalPictureDescription& pictureDesc);
	static bool LoadFromFile(const wxString& strFileName, CPictureDescription& pictureDesc);

	static wxBitmap CreatePicture(const CExternalPictureDescription& pictureDesc, const wxSize& size = wxSize(16, 16));
	static wxBitmap CreatePicture(const CPictureDescription& pictureDesc,
		const class IMetaData* metaData = nullptr, const wxSize& size = wxSize(16, 16));

#pragma region __picture_factory_h__
	static bool IsRegisterPicture(const picture_identifier_t& id);
	static wxBitmap GetPicture(const picture_identifier_t& id);
	static std::vector<CBackendPictureEntry> GetArrayPicture();
#pragma endregion 

#pragma region __picture_conv_h__
	static wxString CreateBase64Image(const wxImage& image);
	static wxImage GetImageFromBase64(const wxString& src);
#pragma endregion 
};

#endif