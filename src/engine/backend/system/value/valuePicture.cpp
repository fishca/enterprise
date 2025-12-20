////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : value picture 
////////////////////////////////////////////////////////////////////////////

#include "valuePicture.h"

//////////////////////////////////////////////////////////////////////
wxIMPLEMENT_DYNAMIC_CLASS(CValuePicture, CValue);

CValuePicture::CValuePicture(const CPictureDescription& pictureDesc) :
	CValue(eValueTypes::TYPE_VALUE), m_pictureDesc(pictureDesc)
{
}

#include <wx/mstream.h>

bool CValuePicture::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	wxFileName filename = paParams[0]->GetString();

	// Cache the image
	if (filename.FileExists()) {

		wxImage image(filename.GetFullPath());
		wxMemoryOutputStream outputStream;

		if (image.SaveFile(outputStream, wxBitmapType::wxBITMAP_TYPE_PNG)) {

			m_pictureDesc.m_type = EPictureType::eFromFile;

			// Get the data from the output stream's buffer
			// The output stream manages the memory buffer internally.
			const size_t buffer_len = outputStream.GetLength();

			m_pictureDesc.m_img_data.m_img_name = filename.GetFullPath();
			m_pictureDesc.m_img_data.m_img_buffer.resize(buffer_len, '\0');

			outputStream.CopyTo(m_pictureDesc.m_img_data.m_img_buffer.data(), buffer_len);

			m_pictureDesc.m_img_data.m_width = image.GetWidth();
			m_pictureDesc.m_img_data.m_height = image.GetHeight();
			return true;
		}
	}

	return false;
}

//**********************************************************************
//*                       Runtime register                             *
//**********************************************************************

VALUE_TYPE_REGISTER(CValuePicture, "storagePicture", string_to_clsid("SY_PICTR"));
