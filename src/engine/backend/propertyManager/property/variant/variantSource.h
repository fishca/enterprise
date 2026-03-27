#ifndef __SOURCE_DATA_VARIANT_H__
#define __SOURCE_DATA_VARIANT_H__

#include "variantType.h"

class BACKEND_API ibVariantDataAttributeSource : public ibVariantDataAttribute {
protected:
	virtual void DoSetFromMetaId(const ibMetaID& id);
	virtual void DoRefreshTypeDesc();
public:

	ibVariantDataAttributeSource(const ibBackendTypeSourceFactory* prop, const ibMetaID& id)
		: ibVariantDataAttribute(prop), m_ownerSrcProperty(prop)
	{
		SetFromMetaDesc(id);
	}

	ibVariantDataAttributeSource(const ibBackendTypeSourceFactory* prop, const ibTypeDescription& typeDesc)
		: ibVariantDataAttribute(prop, typeDesc), m_ownerSrcProperty(prop)
	{
	}

	ibVariantDataAttributeSource(const ibVariantDataAttributeSource& srcData)
		: ibVariantDataAttribute(srcData), m_ownerSrcProperty(srcData.m_ownerSrcProperty)
	{
		RefreshTypeDesc();
	}

	virtual ibVariantDataAttributeSource* Clone() const {
		return new ibVariantDataAttributeSource(*this);
	}

	virtual wxString GetType() const {
		return wxT("ibVariantDataAttributeSource");
	}

	friend class ibVariantDataSource;

protected:
	const ibBackendTypeSourceFactory* m_ownerSrcProperty = nullptr;
};

///////////////////////////////////////////////////////////////////////////

class BACKEND_API ibVariantDataSource : public wxVariantData {
	wxString MakeString() const;
public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsEmptySource() const { return GetIdByGuid(m_dataSource) == wxNOT_FOUND; }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ibVariantDataAttributeSource* CloneSourceAttribute(const ibMetaID& id) const { return new ibVariantDataAttributeSource(m_ownerProperty, id); }
	ibVariantDataAttributeSource* CloneSourceAttribute() const { return new ibVariantDataAttributeSource(*m_attributeSource); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SetSourceAttribute(ibVariantDataAttributeSource* data) {
		m_attributeSource->SetFromMetaDesc(wxNOT_FOUND);
		m_attributeSource->SetFromTypeDesc(data->GetTypeDesc());
	}

	ibVariantDataAttributeSource* GetSourceAttribute() const { return m_attributeSource; }

	//////////////////////////////////////////////////

	ibValueMetaObjectAttributeBase* GetSourceAttributeObject() const;

	//////////////////////////////////////////////////

	void SetSource(const ibMetaID& id, bool fillTypeDesc = true);
	ibMetaID GetSource() const;

	//////////////////////////////////////////////////

	void SetSourceGuid(const ibGuid& guid, bool fillTypeDesc = true);
	ibGuid GetSourceGuid() const;

	//////////////////////////////////////////////////

	void SetSourceTypeDesc(const ibTypeDescription& td);
	ibTypeDescription& GetSourceTypeDesc(bool fillTypeDesc = true) const;

	//////////////////////////////////////////////////

	void ResetSource();

	//////////////////////////////////////////////////

	ibMetaID GetIdByGuid(const ibGuid& guid) const;
	ibGuid GetGuidByID(const ibMetaID& id) const;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsPropAllowed() const;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ibVariantDataSource(const ibBackendTypeSourceFactory* prop, const ibMetaID& id) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(wxNullGuid) {

		m_attributeSource = new ibVariantDataAttributeSource(prop, id);
		//m_attributeSource->IncRef(); // always one 

		m_dataSource = GetGuidByID(id);
	}

	ibVariantDataSource(const ibBackendTypeSourceFactory* prop, const ibGuid& id, bool fillTypeDesc = true) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(id) {

		m_attributeSource = new ibVariantDataAttributeSource(prop, fillTypeDesc ? GetIdByGuid(id) : wxNOT_FOUND);
		//m_attributeSource->IncRef(); // always one 

		//m_dataSource = GetSourceGuid();
	}

	ibVariantDataSource(const ibBackendTypeSourceFactory* prop, const ibTypeDescription& typeDesc) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(prop), m_dataSource(wxNullGuid) {

		m_attributeSource = new ibVariantDataAttributeSource(prop, typeDesc);
		//m_attributeSource->IncRef(); // always one 
	}

	ibVariantDataSource(const ibVariantDataSource& srcData) : wxVariantData(),
		m_attributeSource(nullptr), m_ownerProperty(srcData.m_ownerProperty), m_dataSource(srcData.m_dataSource) {

		m_attributeSource = new ibVariantDataAttributeSource(*srcData.m_attributeSource);
		//m_attributeSource->IncRef(); // always one 
	}

	virtual ~ibVariantDataSource() { m_attributeSource->DecRef(); }

	virtual ibVariantDataSource* Clone() const { return new ibVariantDataSource(*this); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool Eq(wxVariantData& data) const {
		ibVariantDataSource* srcData = dynamic_cast<ibVariantDataSource*>(&data);
		if (srcData != nullptr) {
			ibVariantDataAttribute* srcAttr = srcData->m_attributeSource;
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

	virtual wxString GetType() const { return wxT("ibVariantDataSource"); }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:

	ibGuid m_dataSource;
	const ibBackendTypeSourceFactory* m_ownerProperty = nullptr;
	ibVariantDataAttributeSource* m_attributeSource = nullptr;
};

#endif // !__TYPE_VARIANT_H__
