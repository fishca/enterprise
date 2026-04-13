#ifndef __PROPERTY_PICTURE_H__
#define __PROPERTY_PICTURE_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropPicture.h"

//base property for "picture"
class BACKEND_API CPropertyPicture : public IProperty {
	wxVariantData* CreateVariantData(IPropertyObject* property, const CPictureDescription& id) const;
public:

#pragma region _value_
	wxBitmap GetValueAsBitmap() const;
	CPictureDescription& GetValueAsPictureDesc() const;
	void SetValue(const CPictureDescription& val);
#pragma endregion 

	CPropertyPicture(CPropertyCategory* cat, const wxString& name, const CPictureDescription& id = CPictureDescription())
		: IProperty(cat, name, CreateVariantData(cat->GetPropertyObject(), id))
	{
	}

	CPropertyPicture(CPropertyCategory* cat, const wxString& name, const wxString& label, const CPictureDescription& id = CPictureDescription())
		: IProperty(cat, name, label, CreateVariantData(cat->GetPropertyObject(), id))
	{
	}

	CPropertyPicture(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const CPictureDescription& id = CPictureDescription())
		: IProperty(cat, name, label, helpString, CreateVariantData(cat->GetPropertyObject(), id))
	{
	}

	virtual bool IsEmptyProperty() const;

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGPictureProperty(m_propLabel, m_propName, m_propValue);
	}

	//set/Get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

//base property for "external picture"
class BACKEND_API CPropertyExternalPicture : public IProperty {
	wxVariantData* CreateVariantData(const CExternalPictureDescription& pictureDesc) const;
public:

#pragma region _value_
	wxBitmap GetValueAsBitmap() const;
	CExternalPictureDescription& GetValueAsPictureDesc() const;
	void SetValue(const CExternalPictureDescription& val);
#pragma endregion 

	CPropertyExternalPicture(CPropertyCategory* cat, const wxString& name, const CExternalPictureDescription& pictureDesc = CExternalPictureDescription())
		: IProperty(cat, name, CreateVariantData(pictureDesc))
	{
	}

	CPropertyExternalPicture(CPropertyCategory* cat, const wxString& name, const wxString& label, const CExternalPictureDescription& pictureDesc = CExternalPictureDescription())
		: IProperty(cat, name, label, CreateVariantData(pictureDesc))
	{
	}

	CPropertyExternalPicture(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const CExternalPictureDescription& pictureDesc = CExternalPictureDescription())
		: IProperty(cat, name, label, helpString, CreateVariantData(pictureDesc))
	{
	}

	virtual bool IsEmptyProperty() const;

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxPGExternalImageProperty(m_propLabel, m_propName, m_propValue);
	}

	//set/Get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);
};

#endif