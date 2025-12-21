#ifndef __BACKEND_PICTURE__H__
#define __BACKEND_PICTURE__H__

#include "pictureDescription.h"

class BACKEND_API CBackendPicture {
	CBackendPicture() = delete;
public:

	static bool LoadFromFile(const wxString& strFileName, CExternalPictureDescription& pictureDesc);
	static bool LoadFromFile(const wxString& strFileName, CPictureDescription& pictureDesc);

	static wxBitmap CreatePicture(const CExternalPictureDescription& pictureDesc, const wxSize& size = wxSize(16, 16));
	static wxBitmap CreatePicture(const CPictureDescription& pictureDesc,
		const class IMetaData* metaData = nullptr, const wxSize& size = wxSize(16, 16));
};

#endif