#ifndef __SOURCE_DATA_VARIANT_H__
#define __SOURCE_DATA_VARIANT_H__

#include "typeVariant.h"

class BACKEND_API wxVariantDataAttributeSource : public wxVariantDataAttribute {
protected:
	virtual void DoSetFromMetaId(const meta_identifier_t& id);
	virtual void DoRefreshTypeDesc();
public:

	wxVariantDataAttributeSource(const IBackendTypeSourceFactory* prop, const meta_identifier_t& id)
		: wxVariantDataAttribute(prop), m_ownerSrcProperty(prop)
	{
		SetFromMetaDesc(id);
	}

	wxVariantDataAttributeSource(const IBackendTypeSourceFactory* prop, const CTypeDescription& typeDesc)
		: wxVariantDataAttribute(prop, typeDesc), m_ownerSrcProperty(prop)
	{
	}

	wxVariantDataAttributeSource(const wxVariantDataAttributeSource& srcData)
		: wxVariantDataAttribute(srcData), m_ownerSrcProperty(srcData.m_ownerSrcProperty)
	{
		RefreshTypeDesc();
	}

	virtual wxVariantDataAttributeSource* Clone() const {
		return new wxVariantDataAttributeSource(*this);
	}

	virtual wxString GetType() const {
		return wxT("wxVariantDataAttributeSource");
	}

	friend class wxVariantDataSource;

protected:
	const IBackendTypeSourceFactory* m_ownerSrcProperty = nullptr;
};

///////////////////////////////////////////////////////////////////////////

class BACKEND_API wxVariantDataSource : public wxVariantData {
	wxString MakeString() const;
public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsEmptySource() const { return GetIdByGuid(m_dataSource) == wxNOT_FOUND; }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	wxVariantDataAttributeSource* CloneSourceAttribute(const meta_identifier_t& id) const { return new wxVariantDataAttributeSource(m_ownerProperty, id); }
	wxVariantDataAttributeSource* CloneSourceAttribute() const { return new wxVariantDataAttributeSource(*m_attributeSource); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SetSourceAttribute(wxVariantDataAttributeSource* data) {
		m_attributeSource->SetFromMetaDesc(wxNOT_FOUND);
		m_attributeSource->SetFromTypeDesc(data->GetTypeDesc());
	}

	wxVariantDataAttributeSource* GetSourceAttribute() const { return m_attributeSource; }

	//////////////////////////////////////////////////

	IMetaObjectAttribute* GetSourceAttributeObject() const;

	//////////////////////////////////////////////////

	void SetSource(const meta_identifier_t& id, bool fillTypeDesc = true);
	meta_identifier_t GetSource() const;

	//////////////////////////////////////////////////

	void SetSourceGuid(const CGuid& guid, bool fillTypeDesc = true);
	CGuid GetSourceGuid() const;

	//////////////////////////////////////////////////

	void SetSourceTypeDesc(const CTypeDescription& td);
	CTypeDescription& GetSourceTypeDesc(bool fillTypeDesc = true) const;

	//////////////////////////////////////////////////

	void ResetSource();

	//////////////////////////////////////////////////

	meta_identifier_t GetIdByGuid(const CGuid& guid) const;
	CGuid GetGuidByID(const meta_identifier_t& id) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsPropAllowed() const;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	wxVariantDataSource(const IBackendTypeSourceFactory* prop, const meta_identifier_t& id) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(wxNullGuid) {

		m_attributeSource = new wxVariantDataAttributeSource(prop, id);
		//m_attributeSource->IncRef(); // always one 

		m_dataSource = GetGuidByID(id);
	}

	wxVariantDataSource(const IBackendTypeSourceFactory* prop, const CGuid& id, bool fillTypeDesc = true) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(id) {

		m_attributeSource = new wxVariantDataAttributeSource(prop, fillTypeDesc ? GetIdByGuid(id) : wxNOT_FOUND);
		//m_attributeSource->IncRef(); // always one 

		//m_dataSource = GetSourceGuid();
	}

	wxVariantDataSource(const IBackendTypeSourceFactory* prop, const CTypeDescription& typeDesc) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(wxNullGuid) {

		m_attributeSource = new wxVariantDataAttributeSource(prop, typeDesc);
		//m_attributeSource->IncRef(); // always one 
	}

	wxVariantDataSource(const wxVariantDataSource& srcData) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(srcData.m_ownerProperty), m_dataSource(srcData.m_dataSource) {

		m_attributeSource = new wxVariantDataAttributeSource(*srcData.m_attributeSource);
		//m_attributeSource->IncRef(); // always one 
	}

	virtual ~wxVariantDataSource() { m_attributeSource->DecRef(); }

	virtual wxVariantDataSource* Clone() const {
		return new wxVariantDataSource(*this);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool Eq(wxVariantData& data) const {
		wxVariantDataSource* srcData = dynamic_cast<wxVariantDataSource*>(&data);
		if (srcData != nullptr) {
			wxVariantDataAttribute* srcAttr = srcData->m_attributeSource;
			wxASSERT(srcAttr);
			return m_dataSource == srcData->m_dataSource && srcAttr->Eq(*m_attributeSource);
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	virtual wxString GetType() const { return wxT("wxVariantDataSource"); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:

	CGuid m_dataSource;
	const IBackendTypeSourceFactory* m_ownerProperty = nullptr;
	wxVariantDataAttributeSource* m_attributeSource = nullptr;
};

#endif // !__TYPE_VARIANT_H__
