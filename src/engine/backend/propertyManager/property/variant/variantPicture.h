#ifndef __PICTURE_VARIANT_H__
#define __PICTURE_VARIANT_H__

#include "backend/backend_core.h"
#include "backend/propertyManager/propertyObject.h"
#include "backend/backend_picture.h"

class BACKEND_API wxVariantDataExternalPicture : public wxVariantData {
	wxString MakeString() const;
public:

	bool IsEmptyPicture() const { return m_pictureExternalFile.IsEmptyPicture(); }

	CExternalPictureDescription& GetExternalPictureDesc() { return m_pictureExternalFile; }

	wxString GetPictureFileName() const;
	wxBitmap GetPictureBitmap(const wxSize& size = wxSize(16, 16)) const;

	void SetFromExternalFile(const CExternalPictureDescription& container) { m_pictureExternalFile = container; }

	wxVariantDataExternalPicture(const CExternalPictureDescription& container = CExternalPictureDescription())
		: m_pictureExternalFile(container)
	{
	}

	wxVariantDataExternalPicture(const wxVariantDataExternalPicture& src) :
		wxVariantData(), m_pictureExternalFile(src.m_pictureExternalFile)
	{
	}

	virtual wxVariantDataExternalPicture* Clone() const { return new wxVariantDataExternalPicture(*this); }

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataExternalPicture* srcData = dynamic_cast<wxVariantDataExternalPicture*>(&data);
		if (srcData != nullptr)
			return m_pictureExternalFile == srcData->m_pictureExternalFile;
		return false;
	}

#if wxUSE_STD_IOSTREAM
	virtual bool Write(wxSTD ostream& str) const {
		str << MakeString();
		return true;
	}
#endif

	virtual bool Write(wxString& str) const {
		str = MakeString();
		return true;
	}

	virtual wxString GetType() const { return wxT("wxVariantDataExternalPicture"); }

protected:

	CExternalPictureDescription m_pictureExternalFile;
};

class BACKEND_API wxVariantDataPicture : public wxVariantData {
	wxString MakeString() const;
public:

	bool IsEmptyPicture() const { return m_pictureDesc.IsEmptyPicture(); }

	CPictureDescription& GetPictureDesc() { return m_pictureDesc; }
	wxBitmap GetPictureBitmap(const wxSize& size = wxSize(16, 16)) const;

	wxVariantDataExternalPicture* CloneExternalPicture() const { return m_externalPicture->Clone(); }

	void SetPictureType(EPictureType type) { m_pictureDesc.m_type = type; }

	void SetFromBackendPicture(const class_identifier_t& id) { m_pictureDesc.m_class_identifier = id; }
	void SetFromConfiguraion(const CGuid& id) { m_pictureDesc.m_meta_guid = id; }
	void SetFromExternalFile(const CExternalPictureDescription& container) { m_pictureDesc.m_img_data = container; m_externalPicture->SetFromExternalFile(container); }

	//Get owner value
	const IPropertyObject* GetOwner() { return m_ownerProperty; }

	//set picture data 
	wxVariantDataPicture(const IPropertyObject* prop, const class_identifier_t& id) :
		m_pictureDesc(id), m_ownerProperty(prop)
	{
		m_externalPicture = new wxVariantDataExternalPicture();
	}

	wxVariantDataPicture(const IPropertyObject* prop, const meta_identifier_t& id) :
		m_pictureDesc(id), m_ownerProperty(prop)
	{
		m_externalPicture = new wxVariantDataExternalPicture();
	}

	wxVariantDataPicture(const IPropertyObject* prop, const CExternalPictureDescription& container) :
		m_pictureDesc(container), m_ownerProperty(prop)
	{
		m_externalPicture = new wxVariantDataExternalPicture(container);
	}

	wxVariantDataPicture(const IPropertyObject* prop, const CPictureDescription& pictureDesc) :
		m_pictureDesc(pictureDesc), m_ownerProperty(prop)
	{
		m_externalPicture = new wxVariantDataExternalPicture(pictureDesc.m_img_data);
	}

	wxVariantDataPicture(const wxVariantDataPicture& src) :
		wxVariantData(), m_pictureDesc(src.m_pictureDesc), m_ownerProperty(src.m_ownerProperty)
	{
		m_externalPicture = new wxVariantDataExternalPicture(*src.m_externalPicture);
	}

	virtual ~wxVariantDataPicture() { m_externalPicture->DecRef(); }
	virtual wxVariantDataPicture* Clone() const { return new wxVariantDataPicture(*this); }

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataPicture* srcData = dynamic_cast<wxVariantDataPicture*>(&data);
		if (srcData != nullptr)
			return m_pictureDesc == srcData->m_pictureDesc && m_externalPicture->Eq(*srcData->m_externalPicture);
		return false;
	}

#if wxUSE_STD_IOSTREAM
	virtual bool Write(wxSTD ostream& str) const {
		str << MakeString();
		return true;
	}
#endif

	virtual bool Write(wxString& str) const {
		str = MakeString();
		return true;
	}

	virtual wxString GetType() const { return wxT("wxVariantDataPicture"); }

protected:

	const IPropertyObject* m_ownerProperty = nullptr;
	CPictureDescription m_pictureDesc;
	wxVariantDataExternalPicture* m_externalPicture = nullptr;
};

#endif