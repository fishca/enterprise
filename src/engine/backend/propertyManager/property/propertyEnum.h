#ifndef __PROPERTY_ENUM_H__
#define __PROPERTY_ENUM_H__

#include "backend/propertyManager/propertyObject.h"

//base property for "enum"
class BACKEND_API IPropertyEnum : public IProperty {
public:

	long GetValueAsInteger() const { return m_propValue; }
	void SetValue(const long& i) { m_propValue = i; }

	IPropertyEnum(CPropertyCategory* cat, const wxString& name,
		const wxVariant& value) : IProperty(cat, name, value)
	{
	}

	IPropertyEnum(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const wxVariant& value) : IProperty(cat, name, label, value)
	{
	}

	IPropertyEnum(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const wxVariant& value) : IProperty(cat, name, label, helpString, value)
	{
	}

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const {
		return new wxEnumProperty(m_propLabel, m_propName, GetEnumList(), GetValueAsInteger());
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) = 0;
	virtual bool GetDataValue(CValue& pvarPropVal) const = 0;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer);

protected:
	virtual wxPGChoices GetEnumList() const = 0;
};

#include "backend/compiler/enumUnit.h"

template <typename valEnumProp>
class CPropertyEnum : public IPropertyEnum {
	using valueEnumType = typename valEnumProp::valEnumType;
public:

	valueEnumType GetValueAsEnum() const { return static_cast<valueEnumType>(IPropertyEnum::GetValueAsInteger()); }
	int GetValueAsInteger() const { return IPropertyEnum::GetValueAsInteger(); }
	void SetValue(const valueEnumType& e) { IPropertyEnum::SetValue(static_cast<int>(e)); }
	void SetValue(const int& i) { IPropertyEnum::SetValue(i); }

	CPropertyEnum(CPropertyCategory* cat, const wxString& name,
		const valueEnumType& value) : IPropertyEnum(cat, name, stringUtils::IntToStr(static_cast<int>(value)))
	{
	}

	CPropertyEnum(CPropertyCategory* cat, const wxString& name, const wxString& label,
		const valueEnumType& value) : IPropertyEnum(cat, name, label, stringUtils::IntToStr(static_cast<int>(value)))
	{
	}

	CPropertyEnum(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString,
		const valueEnumType& value) : IPropertyEnum(cat, name, label, helpString, stringUtils::IntToStr(static_cast<int>(value)))
	{
	}

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) {
		SetValue(varPropVal.ConvertToEnumValue<valueEnumType>());
		return true;
	};

	virtual bool GetDataValue(CValue& pvarPropVal) const {
		CValue enumVariant = CPropertyEnum::GetValueAsInteger();
		CValue* ppParams[] = {&enumVariant, nullptr};
		if (m_enumCreator->Init(ppParams, 1)) {
			pvarPropVal = m_enumCreator->GetEnumVariantValue();
			return true;
		}
		pvarPropVal = m_enumCreator.pointer();
		return true;
	};

protected:
	virtual wxPGChoices GetEnumList() const {
		wxPGChoices list;
		for (unsigned int idx = 0; idx < m_enumCreator->GetEnumCount(); idx++) {
			list.Add(m_enumCreator->GetEnumDesc(idx), m_enumCreator->GetEnumValue(idx));
		}
		return list;
	}
private:
	value_ptr<valEnumProp> m_enumCreator = CValue::CreateAndConvertObjectRef<valEnumProp>();
};

#endif