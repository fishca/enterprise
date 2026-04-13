#ifndef __VALUE_PICTURE_H__
#define __VALUE_PICTURE_H__

#include "backend/compiler/value.h"
#include "backend/backend_picture.h"

//picture support
class BACKEND_API CValuePicture : public CValue {
	wxDECLARE_DYNAMIC_CLASS(CValuePicture);
public:

	CValuePicture(const CPictureDescription& pictureDesc = CPictureDescription(0));
	virtual bool Init(CValue** paParams, const long lSizeArray);

	virtual bool IsEmpty() const {
		return m_pictureDesc.IsEmptyPicture();
	}

	const CPictureDescription& GetPictureDesc() const { return m_pictureDesc; }

protected:

	CPictureDescription m_pictureDesc;
};

#endif