#ifndef __PROPERTY_LIST_H__
#define __PROPERTY_LIST_H__

#include "backend/propertyManager/propertyObject.h"
#include "backend/propertyManager/property/advprop/advpropList.h"

//base property for "list"
class BACKEND_API CPropertyList : public IProperty {

	wxPGChoices GetValueList() const {
		wxPGChoices constants;
		if (m_functor->Invoke(const_cast<CPropertyList*>(this))) {
			for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
				wxPGChoiceEntry item(
					m_listPropValue.GetItemLabel(idx),
					m_listPropValue.GetItemId(idx)
				);
				item.SetBitmap(m_listPropValue.GetItemBitmap(idx));
				constants.Add(item);
			}
		}
		return constants;
	}

	class BACKEND_API CPropertyOptionValue {
		enum eValType {
			eValType_pointer,
			eValType_value,
		} m_valType;
		struct {
			CValue* m_pValue, m_cValue;
		};
	public:

		operator CValue* () { return GetOptionValue(); }
		CPropertyOptionValue& operator = (const CPropertyOptionValue& src) {

			if (src.m_valType == eValType::eValType_pointer)
				m_pValue = src.m_pValue;
			else if (src.m_valType == eValType::eValType_value)
				m_cValue = src.m_cValue;

			m_valType = src.m_valType;
			return *this;
		}

		CPropertyOptionValue(CValue* p = nullptr) : m_valType(eValType::eValType_pointer), m_pValue(p) {}
		CPropertyOptionValue(const CValue& v) : m_valType(eValType::eValType_value), m_cValue(v), m_pValue(nullptr) {}

		template <typename T1> CPropertyOptionValue(T1* v) : m_valType(eValType::eValType_pointer), m_pValue(v) {}
		template <typename T1> CPropertyOptionValue(const T1& v) : m_valType(eValType::eValType_value), m_cValue(v), m_pValue(nullptr) {}

		CPropertyOptionValue(const CPropertyOptionValue& val) : m_valType(val.m_valType), m_cValue(val.m_cValue), m_pValue(val.m_pValue) {}
		~CPropertyOptionValue() {}

		CValue* GetOptionValue() {
			return (m_valType == eValType::eValType_pointer) ? m_pValue : new CValue(m_cValue.GetValue());
		}
	};

	class BACKEND_API CPropertyOptionList {

		struct CPropertyOptionItem {
			bool m_isOk;
			wxString m_strName;
			wxString m_strLabel;
			wxString m_strHelp;
			wxBitmap m_bmp;
			long m_id;
			CPropertyOptionValue m_value;
		public:

			CPropertyOptionItem() :
				m_strName(), m_strLabel(), m_id(-1), m_value(), m_isOk(true)
			{
			}

			CPropertyOptionItem(const wxString& name, const long& l, const wxBitmap& b, const CPropertyOptionValue& v) :
				m_strName(name), m_strLabel(name), m_bmp(b), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CPropertyOptionItem(const wxString& name, const wxString& label, const long& l, const wxBitmap& b, const CPropertyOptionValue& v) :
				m_strName(name), m_strLabel(label), m_bmp(b), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CPropertyOptionItem(const wxString& name, const wxString& label, const wxString& help, const long& l, const wxBitmap& b, const CPropertyOptionValue& v) :
				m_strName(name), m_strLabel(label), m_strHelp(help), m_bmp(b), m_id(l), m_value(v), m_isOk(true)
			{
			}

			CPropertyOptionItem(const CPropertyOptionItem& item) :
				m_strName(item.m_strName), m_strLabel(item.m_strLabel), m_strHelp(item.m_strHelp), m_bmp(item.m_bmp), m_id(item.m_id), m_value(item.m_value), m_isOk(true)
			{
			}

			CPropertyOptionItem& operator = (const CPropertyOptionItem& src) {
				m_strName = src.m_strName;
				m_strLabel = src.m_strLabel;
				m_strHelp = src.m_strHelp;
				m_id = src.m_id;
				m_value = src.m_value;
				return *this;
			}

			operator const long() const { return m_id; }
		};

		CPropertyOptionItem GetItemAt(const unsigned int idx) const {
			if (idx > m_listValue.size())
				return CPropertyOptionItem();
			auto it = m_listValue.begin();
			std::advance(it, idx);
			return *it;
		};

		CPropertyOptionItem GetItemById(const long& id) const {
			auto it = std::find_if(m_listValue.begin(), m_listValue.end(),
				[id](const CPropertyOptionItem& p) { return id == p.m_id; }
			);
			if (it != m_listValue.end())
				return *it;
			return CPropertyOptionItem();
		};

	public:

		void ResetListItem() { m_listValue.clear(); }

		void AppendItem(const wxString& name, const int& l, const wxBitmap& b, const CPropertyOptionValue& v) { (void)m_listValue.emplace_back(name, name, l, b, v); }
		void AppendItem(const wxString& name, const wxString& label, const int& l, const wxBitmap& b, const CPropertyOptionValue& v) { (void)m_listValue.emplace_back(name, label, l, b, v); }
		void AppendItem(const wxString& name, const wxString& label, const wxString& help, const int& l, const wxBitmap& b, const CPropertyOptionValue& v) { (void)m_listValue.emplace_back(name, label, help, l, b, v); }

		bool HasValue(const long& l) const { return GetItemById(l); }

		wxString GetItemName(const unsigned int idx) const { return GetItemAt(idx).m_strName; }
		wxString GetItemLabel(const unsigned int idx) const { return GetItemAt(idx).m_strLabel; }
		wxString GetItemHelp(const unsigned int idx) const { return GetItemAt(idx).m_strHelp; }
		wxBitmap GetItemBitmap(const unsigned int idx) const { return GetItemAt(idx).m_bmp; }
		long GetItemId(const unsigned int idx) const { return GetItemAt(idx).m_id; }
		CValue* GetItemValue(const unsigned int idx) const { return GetItemAt(idx).m_value; }

		unsigned int GetItemCount() const { return (unsigned int)m_listValue.size(); }

	private:
		std::vector<CPropertyOptionItem> m_listValue;
	};

	class BACKEND_API CPropertyFunctor {
	public:
		virtual ~CPropertyFunctor() {}
		virtual bool Invoke(CPropertyList* property) = 0;
	};

	template <typename optClass>
	class CPropertyValueFunctor : public CPropertyFunctor {
		bool (optClass::* m_funcHandler)(CPropertyList* prop);
	public:
		CPropertyValueFunctor(bool (optClass::* funcHandler)(CPropertyList* prop), optClass* handler)
			: m_funcHandler(funcHandler), m_handler(handler)
		{
		}
		virtual bool Invoke(CPropertyList* property) override {
			const CPropertyOptionList listPropValue = property->m_listPropValue;
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
		const long sel = m_propValue;
		if (m_functor->Invoke(const_cast<CPropertyList*>(this))) {
			for (unsigned int idx = 0; idx < m_listPropValue.GetItemCount(); idx++) {
				if (m_listPropValue.GetItemId(idx) == sel) {
					return sel;
				}
			}
		}
		return wxNOT_FOUND;
	}

#pragma region item 
	void AppendItem(const wxString& name, const int& l, const wxBitmap& b, const CPropertyOptionValue& v = CPropertyOptionValue()) { (void)m_listPropValue.AppendItem(name, l, b, v); }
	void AppendItem(const wxString& name, const wxString& label, const int& l, const wxBitmap& b, const CPropertyOptionValue& v = CPropertyOptionValue()) { (void)m_listPropValue.AppendItem(name, label, l, b, v); }
	void AppendItem(const wxString& name, const wxString& label, const wxString& help, const int& l, const wxBitmap& b, const CPropertyOptionValue& v = CPropertyOptionValue()) { (void)m_listPropValue.AppendItem(name, label, help, l, b, v); }
#pragma endregion

	template <typename optClass>
	CPropertyList(CPropertyCategory* cat, const wxString& name,
		bool (optClass::* funcHandler)(CPropertyList* prop), const long& value = wxNOT_FOUND) : IProperty(cat, name, value)
	{
		m_functor = new CPropertyValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	template <typename optClass>
	CPropertyList(CPropertyCategory* cat, const wxString& name, const wxString& label,
		bool (optClass::* funcHandler)(CPropertyList* prop), const long& value = wxNOT_FOUND) : IProperty(cat, name, label, value)
	{
		m_functor = new CPropertyValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	template <typename optClass>
	CPropertyList(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		bool (optClass::* funcHandler)(CPropertyList* prop), const long& value = wxNOT_FOUND) : IProperty(cat, name, label, helpString, value)
	{
		m_functor = new CPropertyValueFunctor<optClass>(funcHandler, (optClass*)cat->GetPropertyObject());
	}

	virtual ~CPropertyList() { wxDELETE(m_functor); }

	virtual bool IsEmptyProperty() const { return GetValueAsInteger() == wxNOT_FOUND; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		if (!m_functor->Invoke(const_cast<CPropertyList*>(this)))
			return nullptr;
		return new wxPGListProperty(m_propLabel, m_propName, GetValueList(), GetValueAsInteger());
	}

	// Set/Get property data
	virtual bool SetDataValue(const CValue& varPropVal);
	virtual bool GetDataValue(CValue& pvarPropVal) const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);

protected:

	virtual void DoSetValue(const wxVariant& val) {
		if (m_functor != nullptr) m_functor->Invoke(this);
		IProperty::DoSetValue(val);
	}

private:
	CPropertyOptionList m_listPropValue;
	CPropertyFunctor* m_functor;
};

#endif