#ifndef __SRC_EXPLORER_H__
#define __SRC_EXPLORER_H__

class BACKEND_API CSourceExplorer {

	struct CSourceInfo {

		wxString m_srcName;
		wxString m_srcSynonym;

		ibMetaID m_mid;

		bool m_enabled = true;
		bool m_visible = true;
		bool m_tableSection = false;
		bool m_select = true;

		ibTypeDescription m_typeDesc;

		const ibValueMetaObject* m_metaObject = nullptr;
	};

private:

	CSourceExplorer() {}

	CSourceExplorer(const ibValueMetaObjectCompositeData* object, const ibClassID& cid) {

		m_sourceInfo = {
			wxT("ref"), _("Ref"), object->GetMetaID(), true, false, false, true, { cid }, object
		};

		for (const auto child : object->GetGenericAttributeArrayObject()) CSourceExplorer::AppendSource(child);
	}

	CSourceExplorer(const ibValueMetaObjectAttributeBase* object, bool enabled = true, bool visible = true) {

		m_sourceInfo = {
			object->GetName(), object->GetSynonym(), object->GetMetaID(), enabled, visible, false, true, object->GetTypeDesc(), object
		};
	}

	CSourceExplorer(const ibValueMetaObjectTableData* object) {

		m_sourceInfo = {
			object->GetName(), object->GetSynonym(), object->GetMetaID(), true, true, true, true,  object->GetTypeDesc(), object
		};

		for (const auto child : object->GetGenericAttributeArrayObject()) CSourceExplorer::AppendSource(child);
	}

public:

	CSourceExplorer(const ibValueMetaObject* object, const ibClassID& cid, bool tableSection, bool select = false) {

		m_sourceInfo = {
			wxT("ref"), _("Ref"), object->GetMetaID(), true, true, tableSection, select, cid, object
		};
	}

	// this object 
	CSourceExplorer(const ibValueMetaObjectGenericData* object, const ibClassID& cid, bool tableSection, bool select = false) {

		if (object->IsDeleted())
			return;

		m_sourceInfo = {
			object->GetName(), object->GetSynonym(), object->GetMetaID(), true, true, tableSection, select, cid, object
		};
	}

	wxString GetSourceName() const { return m_sourceInfo.m_srcName; }
	wxString GetSourceSynonym() const { return m_sourceInfo.m_srcSynonym; }

	ibMetaID GetSourceId() const { return m_sourceInfo.m_mid; }

	bool IsEnabled() const { return m_sourceInfo.m_enabled; }
	bool IsVisible() const { return m_sourceInfo.m_visible; }
	bool IsTableSection() const { return m_sourceInfo.m_tableSection; }
	bool IsSelect() const { return m_sourceInfo.m_select && IsAllowed(); }
	bool IsAllowed() const { return GetMetaObject()->IsAllowed(); }

	const ibValueMetaObject* GetMetaObject() const { return m_sourceInfo.m_metaObject; }
	const std::vector<ibClassID>& GetClsidList() const { return m_sourceInfo.m_typeDesc.GetClsidList(); }

	bool ContainType(const ibValueTypes& valType) const { return m_sourceInfo.m_typeDesc.ContainType(valType); }
	bool ContainType(const ibClassID& cid) const { return m_sourceInfo.m_typeDesc.ContainType(cid); }

	void AppendSource(ibValueMetaObjectGenericData* refData, const ibClassID& cid) {
		if (refData->IsDeleted())
			return;
		m_arraySource.emplace_back(CSourceExplorer{ refData, cid });
	}

	void AppendSource(ibValueMetaObjectAttributeBase* attribute, bool enabled = true, bool visible = true) {
		if (attribute->IsDeleted())
			return;
		m_arraySource.emplace_back(CSourceExplorer{ attribute, enabled , visible });
	}

	void AppendSource(ibValueMetaObjectTableData* tableSection) {
		
		if (tableSection->IsDeleted())
			return;
		
		m_arraySource.emplace_back(CSourceExplorer{ tableSection });
	}

	CSourceExplorer GetHelper(unsigned int idx) const {
		
		if (m_arraySource.size() < idx)
			return CSourceExplorer();
		
		return m_arraySource[idx];
	}

	unsigned int GetHelperCount() const { return m_arraySource.size(); }

protected:

	CSourceInfo m_sourceInfo;

	std::vector<CSourceExplorer> m_arraySource;
};

#include "backend/srcObject.h"

#endif 