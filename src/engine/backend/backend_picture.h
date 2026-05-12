#ifndef __BACKEND_PICTURE_H__
#define __BACKEND_PICTURE_H__

#include "pictureDescription.h"
#include "picturePredefined.h"

struct ibBackendPictureEntry {
	wxString m_name;
	wxBitmap m_data;
	ibPictureID m_id;
};

class BACKEND_API ibBackendPicture {
	ibBackendPicture() = delete;
public:

	static void RegisterPicture(const wxString name, const ibPictureID& id, const wxBitmap& bitmap);

	static bool LoadFromFile(const wxString& strFileName, ibExternalPictureDescription& pictureDesc);
	static bool LoadFromFile(const wxString& strFileName, ibPictureDescription& pictureDesc);

	static wxBitmap CreatePicture(const ibExternalPictureDescription& pictureDesc, const wxSize& size = wxSize(16, 16));
	static wxBitmap CreatePicture(const ibPictureDescription& pictureDesc,
		const class ibMetaData* metaData = nullptr, const wxSize& size = wxSize(16, 16));

#pragma region __picture_factory_h__
	static bool IsRegisterPicture(const ibPictureID& id);
	static wxBitmap GetPicture(const ibPictureID& id);
	static wxIcon GetPictureAsIcon(const ibPictureID& id);
	static std::vector<ibBackendPictureEntry> GetArrayPicture();
#pragma endregion 

#pragma region __picture_conv_h__
	static wxString CreateBase64Image(const wxImage& image);
	static wxImage GetImageFromBase64(const wxString& src, const wxSize& size = wxDefaultSize);
	static wxBitmap GetBitmapFromBase64(const wxString& src, const wxSize& size = wxDefaultSize);
	static wxIcon GetIconFromBase64(const wxString& src, const wxSize& size = wxDefaultSize);
#pragma endregion 
};

#endif