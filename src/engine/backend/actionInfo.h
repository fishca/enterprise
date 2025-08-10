#ifndef __ACTION_INFO_H__
#define __ACTION_INFO_H__

class CActionDescription {
	wxString m_strAction;
	action_identifier_t m_lAction;
public:

	bool operator == (const CActionDescription& rhs) const { 
		if (m_lAction == wxNOT_FOUND) 
			return m_strAction == rhs.m_strAction;
		return m_lAction == rhs.m_lAction; 
	}

	CActionDescription(const action_identifier_t& lAction) :m_strAction(), m_lAction(lAction) {}
	CActionDescription(const wxString& strAction) :m_strAction(strAction), m_lAction(wxNOT_FOUND) {}

	wxString GetCustomAction() const { return m_strAction; }
	action_identifier_t GetSystemAction() const { return m_lAction; }

	friend class CActionDescriptionMemory;
};

class CActionDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CActionDescription& typeDesc);
	static bool SaveData(class CMemoryWriter& writer, CActionDescription& typeDesc);
};

class IActionDataObject {
protected:
	class CActionCollection {
		CValue* m_srcData;
		struct CAction {
			action_identifier_t m_act_id;
			wxString m_name;
			wxString m_caption;
			wxBitmap m_bitmap;
			bool m_createDef;
			CValue* m_srcData;
			CAction() : m_name(wxEmptyString),
				m_caption(wxEmptyString),
				m_bitmap(wxNullBitmap),
				m_act_id(wxNOT_FOUND) {
			}
			CAction(const action_identifier_t& act_id, const wxString& name, const wxString& description, const wxBitmap& bitmap, bool createDef, CValue* srcData) :
				m_act_id(act_id), m_name(name), m_caption(description), m_bitmap(bitmap), m_createDef(createDef), m_srcData(srcData) {
			}
		};
		std::vector<CAction> m_vecAction;
	private:

		bool find_act(const action_identifier_t& lNumAction) const {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id;
				}
			);
			return it == m_vecAction.end();
		}

	public:

		CValue* GetSourceData() const { return m_srcData; }

		CActionCollection(CValue* srcData = nullptr) : m_srcData(srcData) {}

		void AddAction(const wxString& name, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.emplace_back(
				lNumAction,
				name,
				wxEmptyString,
				wxNullBitmap,
				createDef,
				srcData ? srcData : m_srcData
			);
		}

		void AddAction(const wxString& name, const wxString& caption, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.emplace_back(
				lNumAction,
				name,
				caption,
				wxNullBitmap,
				createDef,
				srcData ? srcData : m_srcData
			);
		}

		void AddAction(const wxString& name, const wxString& caption, const wxBitmap& bitmap, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.emplace_back(
				lNumAction,
				name,
				caption,
				bitmap,
				createDef,
				srcData ? srcData : m_srcData
			);
		}

		void AddSeparator() {
			m_vecAction.emplace_back();
		}

		void InsertAction(unsigned int index, const wxString& name, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.insert(m_vecAction.begin() + index, {
				lNumAction,
				name,
				wxEmptyString,
				wxNullBitmap,
				createDef,
				srcData ? srcData : m_srcData
				}
			);
		}

		void InsertAction(unsigned int index, const wxString& name, const wxString& caption, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.insert(m_vecAction.begin() + index, {
				lNumAction,
				name,
				caption,
				wxNullBitmap,
				createDef,
				srcData ? srcData : m_srcData
				}
			);
		}

		void InsertAction(unsigned int index, const wxString& name, const wxString& caption, const wxBitmap& bitmap, const action_identifier_t& lNumAction, bool createDef = true, CValue* srcData = nullptr) {
			wxASSERT(find_act(lNumAction));
			m_vecAction.insert(m_vecAction.begin() + index, {
				lNumAction,
				name,
				caption,
				bitmap,
				createDef,
				srcData ? srcData : m_srcData
				}
			);
		}

		void InsertSeparator(unsigned int index) {
			m_vecAction.insert(m_vecAction.begin() + index, {});
		}

		void RemoveAction(const action_identifier_t& lNumAction) {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id; }
			);
			if (it != m_vecAction.end()) {
				m_vecAction.erase(it);
			}
		}

		wxString GetNameByID(const action_identifier_t& lNumAction) const {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id; });
			if (it != m_vecAction.end())
				return it->m_name;
			return wxEmptyString;
		}

		wxString GetCaptionByID(const action_identifier_t& lNumAction) const {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id; }
			);
			if (it != m_vecAction.end()) {
				wxString caption = it->m_caption;
				return caption.Length() > 0 ?
					caption : it->m_name;
			}
			return wxEmptyString;
		}

		bool IsCreateInForm(const action_identifier_t& lNumAction) const {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id; }
			);
			if (it != m_vecAction.end())
				return it->m_createDef;

			return true;
		}

		CValue* GetSourceDataByID(const action_identifier_t& lNumAction) const {
			auto& it = std::find_if(m_vecAction.begin(), m_vecAction.end(), [lNumAction](const CAction& act) {
				return lNumAction == act.m_act_id; }
			);
			if (it != m_vecAction.end())
				return it->m_srcData;
			return nullptr;
		}

		action_identifier_t GetID(unsigned int idx) const {
			if (idx > GetCount())
				return wxNOT_FOUND;
			return m_vecAction[idx].m_act_id;
		}

		unsigned int GetCount() const {
			return m_vecAction.size();
		}
	};

public:

	//support action 
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType) = 0;
	virtual void AppendActionCollection(CActionCollection& actionData, const form_identifier_t& formType) {
		const CActionCollection& data = GetActionCollection(formType);
		for (unsigned int i = 0; i < data.GetCount(); i++) {
			const action_identifier_t& id = data.GetID(i);
			if (id != wxNOT_FOUND) {
				actionData.AddAction(
					data.GetNameByID(id),
					data.GetCaptionByID(id),
					id,
					data.IsCreateInForm(id),
					data.GetSourceDataByID(id)
				);
			}
			else {
				actionData.AddSeparator();
			}
		}
	}

	// execute action 
	virtual void ExecuteAction(const action_identifier_t& lNumAction, class IBackendValueForm* srcForm) = 0;
};

#endif 