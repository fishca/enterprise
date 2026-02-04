#ifndef __META_CONTEXT_H__
#define __META_CONTEXT_H__

#include "backend/metaCollection/attribute/metaAttributeObject.h"

#pragma region __property_standart_h__

//base property for "inner attribute"
template <typename T = class CValueMetaObjectAttributePredefined>
class CPropertyInnerAttribute : public IProperty {
public:

	T* GetMetaObject() const { return m_metaObject; }

	CPropertyInnerAttribute(CPropertyCategory* cat, T* metaObject)
		: IProperty(cat, metaObject->GetName(), metaObject->GetSynonym(), wxNullVariant), m_metaObject(metaObject)
	{
	}

	virtual ~CPropertyInnerAttribute() {}

	// get meta object 
	T* operator->() { return GetMetaObject(); }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const { return nullptr; }

	// set/get property data
	virtual bool SetDataValue(const CValue& varPropVal) { return false; }
	virtual bool GetDataValue(CValue& pvarPropVal) const {
		pvarPropVal = m_metaObject;
		return true;
	}

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) { return false; }
	virtual bool SaveData(CMemoryWriter& writer) { return false; }

	//copy & paste object in control 
	virtual bool PasteData(CMemoryReader& reader) {
		m_metaObject->SetCommonGuid(reader.r_stringZ());
		return true;
	}

	virtual bool CopyData(CMemoryWriter& writer) {
		writer.w_stringZ(m_metaObject->GetCommonGuid());
		return true;
	}

private:

	CValuePtr<T> m_metaObject;
};

#pragma endregion

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

// type ctor for meta
class IMetaValueTypeCtor;

//********************************************************************************************
//*                                  Factory & metadata                                      *
//********************************************************************************************


class BACKEND_API IValueMetaObjectCompositeData
	: public IValueMetaObject {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectCompositeData);
public:

	IValueMetaObjectCompositeData(
		const wxString& strName = wxEmptyString,
		const wxString& synonym = wxEmptyString,
		const wxString& comment = wxEmptyString
	) : IValueMetaObject(strName, synonym, comment) {
	}

	virtual ~IValueMetaObjectCompositeData() {}

	//guid to id 
	meta_identifier_t IValueMetaObjectCompositeData::GetIdByGuid(const CGuid& guid) const {
		IValueMetaObject* metaObject = FindAnyObjectByFilter(guid);
		if (metaObject != nullptr)
			return metaObject->GetMetaID();
		return wxNOT_FOUND;
	}

	CGuid IValueMetaObjectCompositeData::GetGuidByID(const meta_identifier_t& id) const {
		IValueMetaObject* metaObject = FindAnyObjectByFilter(id);
		if (metaObject != nullptr)
			return metaObject->GetGuid();
		return wxNullGuid;
	}

	//runtime support:
	const IMetaValueTypeCtor* GetTypeCtor(const enum eCtorMetaType& refType) const;

#pragma region __generic_h__

	//any attribute 
	virtual std::vector<IValueMetaObjectAttribute*> GetGenericAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const = 0;

#pragma endregion
#pragma region __array_h__

	//predefined 
	std::vector<IValueMetaObjectAttribute*> GetPredefinedAttributeArrayObject(
		std::vector<IValueMetaObjectAttribute*>& array = std::vector<IValueMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		return array;
	}

#pragma endregion 
#pragma region __filter_h__

	//predefined 
	template <typename _T1>
	IValueMetaObjectAttribute* FindPredefinedAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IValueMetaObjectAttribute>(id, { g_metaPredefinedAttributeCLSID });
	}

#pragma endregion 

protected:

	CValueMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, false, eValueTypes::TYPE_BOOLEAN, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, eValueTypes::TYPE_BOOLEAN, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, const bool& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, CValue(defValue), useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), false, eValueTypes::TYPE_NUMBER, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), fillCheck, eValueTypes::TYPE_NUMBER, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, bool fillCheck, const number_t& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), fillCheck, CValue(defValue), useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), false, eValueTypes::TYPE_DATE, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), fillCheck, eValueTypes::TYPE_DATE, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, bool fillCheck, const wxDateTime& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), fillCheck, CValue(defValue), useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), false, eValueTypes::TYPE_STRING, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), fillCheck, eValueTypes::TYPE_STRING, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, bool fillCheck, const wxString& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), fillCheck, CValue(defValue), useItem, selectMode);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CValueMetaObjectAttributePredefined* CreateEmptyType(const wxString& name, const wxString& synonym, const wxString& comment,
		eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, false, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateEmptyType(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, useItem, selectMode);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CValueMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, clsid, false, CValue(), useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, clsid, false, defValue, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, bool fillCheck, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, clsid, fillCheck, defValue, useItem, selectMode);
	}

	CValueMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr,
		bool fillCheck, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IValueMetaObject::CreateMetaObjectAndSetParent<CValueMetaObjectAttributePredefined>(name, synonym, comment, clsid, descr, fillCheck, defValue, useItem, selectMode);
	}

	virtual bool FillArrayObjectByPredefined(
		std::vector<IValueMetaObjectAttribute*>& array) const {
		return false;
	}
};

#endif