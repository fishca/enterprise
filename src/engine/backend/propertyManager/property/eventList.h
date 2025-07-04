#ifndef __EVENT_LIST_H__
#define __EVENT_LIST_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropEventTool.h"

//base event for "list"
class BACKEND_API CEventList : public IEvent {

	wxPGChoices GetEventList() const {
		wxPGChoices constants;
		for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
			wxPGChoiceEntry item(
				m_listPropValue.GetItemName(idx),
				m_listPropValue.GetItemId(idx)
			);
			constants.Add(item);
		}
		return constants;
	}

	class BACKEND_API CEventOptionList {

		struct CEventOptionItem {
			bool m_isOk;
			wxString m_strName;
			wxString m_strLabel;
			wxString m_strHelp;
			long m_id;
			CValue m_value;
		public:

			CEventOptionItem() :
				m_strName(), m_strLabel(), m_id(-1), m_value(), m_isOk(true)
			{
			}

			CEventOptionItem(const wxString& name, const long& l, const CValue& v) :
				m_strName(name), m_strLabel(name), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CEventOptionItem(const wxString& name, const wxString& label, const long& l, const CValue& v) :
				m_strName(name), m_strLabel(label), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CEventOptionItem(const wxString& name, const wxString& label, const wxString& help, const long& l, const CValue& v) :
				m_strName(name), m_strLabel(label), m_strHelp(help), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CEventOptionItem(const CEventOptionItem& item) :
				m_strName(item.m_strName), m_strLabel(item.m_strLabel), m_strHelp(item.m_strHelp), m_id(item.m_id), m_value(item.m_value), m_isOk(true)
			{
			}

			CEventOptionItem& operator = (const CEventOptionItem& src) {
				m_strName = src.m_strName;
				m_strLabel = src.m_strLabel;
				m_strHelp = src.m_strHelp;
				m_id = src.m_id;
				m_value = src.m_value;
				return *this;
			}

			operator const long() const { return m_id; }
		};

		CEventOptionItem GetItemAt(const unsigned int idx) const {
			if (idx > m_listValue.size())
				return CEventOptionItem();
			auto it = m_listValue.begin();
			std::advance(it, idx);
			return *it;
		};

		CEventOptionItem GetItemById(const long& id) const {
			auto it = std::find_if(m_listValue.begin(), m_listValue.end(),
				[id](const CEventOptionItem& p) { return id == p.m_id; }
			);
			if (it != m_listValue.end())
				return *it;
			return CEventOptionItem();
		};

	public:

		void ResetListItem() { m_listValue.clear(); }

		void AppendItem(const wxString& name, const int& l, const CValue& v) { (void)m_listValue.emplace_back(name, l, v); }
		void AppendItem(const wxString& name, const wxString& label, const int& l, const CValue& v) { (void)m_listValue.emplace_back(name, label, l, v); }
		void AppendItem(const wxString& name, const wxString& label, const wxString& help, const int& l, const CValue& v) { (void)m_listValue.emplace_back(label, help, l, v); }

		bool HasValue(const long& l) const { return GetItemById(l); }

		wxString GetItemName(const unsigned int idx) const { return GetItemAt(idx).m_strName; }
		wxString GetItemLabel(const unsigned int idx) const { return GetItemAt(idx).m_strLabel; }
		wxString GetItemHelp(const unsigned int idx) const { return GetItemAt(idx).m_strHelp; }
		long GetItemId(const unsigned int idx) const { return GetItemAt(idx).m_id; }
		CValue GetItemValue(const unsigned int idx) const { return GetItemAt(idx).m_value; }

		unsigned int GetItemCount() const { return (unsigned int)m_listValue.size(); }

	private:
		std::vector<CEventOptionItem> m_listValue;
	};

	class BACKEND_API CEventFunctor {
	public:
		virtual bool Invoke(CEventList* property) = 0;
	};

	template <typename optClass>
	class CEventValueFunctor : public CEventFunctor {
		bool (optClass::* m_funcHandler)(CEventList* evt);
	public:
		CEventValueFunctor(bool (optClass::* funcHandler)(CEventList* evt), optClass* handler)
			: m_funcHandler(funcHandler), m_handler(handler)
		{
		}
		virtual bool Invoke(CEventList* property) override {
			const CEventOptionList listPropValue = property->m_listPropValue;
			if (property != nullptr) property->ResetListItem();
			return (m_handler->*m_funcHandler)(property);
		}
	private:
		optClass* m_handler;
	};
#pragma region item 
	void ResetListItem() { (void)m_listPropValue.ResetListItem(); }
#pragma endregion
public:
	int GetValueAsInteger() const {
		const int sel = typeConv::StringToInt(m_propValue);
		if (m_functor->Invoke(const_cast<CEventList*>(this))) {
			for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
				if (m_listPropValue.GetItemId(idx) == sel) {
					return sel;
				}
			}
		}
		return wxNOT_FOUND;
	}
#pragma region item 
	void AppendItem(const wxString& name, const int& l, const CValue& v) { (void)m_listPropValue.AppendItem(name, l, v); }
	void AppendItem(const wxString& name, const wxString& label, const int& l, const CValue& v) { (void)m_listPropValue.AppendItem(name, label, l, v); }
	void AppendItem(const wxString& name, const wxString& label, const wxString& help, const int& l, const CValue& v) { (void)m_listPropValue.AppendItem(name, label, help, l, v); }
#pragma endregion

	template <typename optClass>
	CEventList(CPropertyCategory* cat, const wxString& name, const wxArrayString& args,
		bool (optClass::* funcHandler)(CEventList* evt), const int& value) : IEvent(cat, name, args, stringUtils::IntToStr(value))
	{
		m_functor = new CEventValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	template <typename optClass>
	CEventList(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxArrayString& args,
		bool (optClass::* funcHandler)(CEventList* evt), const int& value) : IEvent(cat, name, label, args, stringUtils::IntToStr(value))
	{
		m_functor = new CEventValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	template <typename optClass>
	CEventList(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxArrayString& args,
		bool (optClass::* funcHandler)(CEventList* evt), const int& value) : IEvent(cat, name, label, helpString, args, stringUtils::IntToStr())
	{
		m_functor = new CEventValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	virtual ~CEventList() { wxDELETE(m_functor); }

	virtual bool IsEmptyProperty() const { return GetValueAsInteger() == wxNOT_FOUND; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		if (!m_functor->Invoke(const_cast<CEventList*>(this)))
			return nullptr;
		return new wxEventToolProperty(m_propLabel, m_propName, GetEventList(), m_propValue);
	}

	// Set/Get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);

private:
	CEventOptionList m_listPropValue;
	CEventFunctor* m_functor;
};

#endif