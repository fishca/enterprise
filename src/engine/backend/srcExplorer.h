#ifndef __SRC_EXPLORER_H__
#define __SRC_EXPLORER_H__

class BACKEND_API CSourceExplorer {
	struct CSourceData {
		wxString m_srcName;
		wxString m_srcSynonym;
		IMetaObject* m_metaObject = nullptr;
		meta_identifier_t m_meta_id = wxNOT_FOUND;
		CTypeDescription m_typeDesc;
		bool m_enabled = true;
		bool m_visible = true;
		bool m_tableSection = false;
		bool m_select = true;
	};
private:

	CSourceExplorer() {}

	CSourceExplorer(IMetaObjectCompositeData* refData, const class_identifier_t& clsid) {
		m_srcData = { wxT("ref"), _("ref"), refData, refData->GetMetaID(), { clsid }, true, false };

		for (const auto object : refData->GetGenericAttributeArrayObject()) {
			CSourceExplorer::AppendSource(object);
		}
	}

	CSourceExplorer(IMetaObjectAttribute* attribute, bool enabled = true, bool visible = true) {
		m_srcData = { attribute->GetName(), attribute->GetSynonym(), attribute, attribute->GetMetaID(), attribute->GetTypeDesc(), enabled, visible, false };
	}

	CSourceExplorer(CMetaObjectTableData* tableSection) {
		m_srcData = { tableSection->GetName(), tableSection->GetSynonym(), tableSection, tableSection->GetMetaID(), tableSection->GetTypeDesc(), true, true, true };
		for (const auto object : tableSection->GetGenericAttributeArrayObject()) {
			CSourceExplorer::AppendSource(object);
		}
	}

public:

	CSourceExplorer(IMetaObject* refData, const class_identifier_t& clsid, bool tableSection, bool select = false) {
		m_srcData = { wxT("ref"), _("ref"), refData, refData->GetMetaID(), clsid, true, true, tableSection, select };
	}

	// this object 
	CSourceExplorer(IMetaObjectGenericData* refData, const class_identifier_t& clsid, bool tableSection, bool select = false) {
		if (refData->IsDeleted())
			return;
		m_srcData = { refData->GetName(), refData->GetSynonym(), refData, refData->GetMetaID(), clsid,  true, true, tableSection, select };
	}

	wxString GetSourceName() const { return m_srcData.m_srcName; }
	wxString GetSourceSynonym() const { return m_srcData.m_srcSynonym; }
	meta_identifier_t GetSourceId() const { return m_srcData.m_meta_id; }

	bool IsEnabled() const { return m_srcData.m_enabled; }
	bool IsVisible() const { return m_srcData.m_visible; }
	bool IsTableSection() const { return m_srcData.m_tableSection; }
	bool IsSelect() const { return m_srcData.m_select && IsAllowed(); }
	bool IsAllowed() const { return GetMetaObject()->IsAllowed(); }

	IMetaObject* GetMetaObject() const { return m_srcData.m_metaObject; }

	const std::vector<class_identifier_t>& GetClsidList() const { return m_srcData.m_typeDesc.GetClsidList(); }

	bool ContainType(const eValueTypes& valType) const { return m_srcData.m_typeDesc.ContainType(valType); }
	bool ContainType(const class_identifier_t& clsid) const { return m_srcData.m_typeDesc.ContainType(clsid); }

	void AppendSource(IMetaObjectGenericData* refData, const class_identifier_t& clsid) {
		if (refData->IsDeleted())
			return;
		m_listSrcData.emplace_back(
			CSourceExplorer{ refData, clsid }
		);
	}

	void AppendSource(IMetaObjectAttribute* attribute, bool enabled = true, bool visible = true) {
		if (attribute->IsDeleted())
			return;
		m_listSrcData.emplace_back(
			CSourceExplorer{ attribute, enabled , visible }
		);
	}

	void AppendSource(CMetaObjectTableData* tableSection) {
		if (tableSection->IsDeleted())
			return;
		m_listSrcData.emplace_back(
			CSourceExplorer{ tableSection }
		);
	}

	CSourceExplorer GetHelper(unsigned int idx) const {
		if (m_listSrcData.size() < idx) {
			return CSourceExplorer();
		}
		return m_listSrcData[idx];
	}

	unsigned int GetHelperCount() const { return m_listSrcData.size(); }

protected:

	std::vector<CSourceExplorer> m_listSrcData;
	CSourceData m_srcData;
};

#include "backend/srcObject.h"

#endif 