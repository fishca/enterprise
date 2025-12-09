#ifndef __META_CONTEXT_H__
#define __META_CONTEXT_H__

#include "backend/metaCollection/attribute/metaAttributeObject.h"

#pragma region __property_standart_h__

//base property for "inner attribute"
template <typename T = class CMetaObjectAttributePredefined>
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

class BACKEND_API IMetaObjectCompositeData
	: public IMetaObject {
	wxDECLARE_ABSTRACT_CLASS(IMetaObjectCompositeData);
public:

	IMetaObjectCompositeData(
		const wxString& strName = wxEmptyString,
		const wxString& synonym = wxEmptyString,
		const wxString& comment = wxEmptyString
	) : IMetaObject(strName, synonym, comment) {
	}

	virtual ~IMetaObjectCompositeData() {}

	//guid to id 
	meta_identifier_t IMetaObjectCompositeData::GetIdByGuid(const CGuid& guid) const {
		IMetaObject* metaObject = FindAnyObjectByFilter(guid);
		if (metaObject != nullptr)
			return metaObject->GetMetaID();
		return wxNOT_FOUND;
	}

	CGuid IMetaObjectCompositeData::GetGuidByID(const meta_identifier_t& id) const {
		IMetaObject* metaObject = FindAnyObjectByFilter(id);
		if (metaObject != nullptr)
			return metaObject->GetGuid();
		return wxNullGuid;
	}

	//runtime support:
	const IMetaValueTypeCtor* GetTypeCtor(const enum eCtorMetaType& refType) const;

#pragma region __generic_h__

	//any attribute 
	virtual std::vector<IMetaObjectAttribute*> GetGenericAttributeArrayObject(
		std::vector<IMetaObjectAttribute*>& array = std::vector<IMetaObjectAttribute*>()) const = 0;

#pragma endregion
#pragma region __array_h__

	//any
	std::vector<IMetaObject*> GetAnyArrayObject(
		std::vector<IMetaObject*>& array = std::vector<IMetaObject*>()) const {
		FillArrayObjectByFilter<IMetaObject>(array, {});
		return array;
	}

	//predefined 
	std::vector<IMetaObjectAttribute*> GetPredefinedAttributeArrayObject(
		std::vector<IMetaObjectAttribute*>& array = std::vector<IMetaObjectAttribute*>()) const {
		FillArrayObjectByPredefined(array);
		return array;
	}

#pragma endregion 
#pragma region __filter_h__

	//any 
	template <typename _T1>
	IMetaObject* FindAnyObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IMetaObject>(id, {});
	}

	//predefined 
	template <typename _T1>
	IMetaObjectAttribute* FindPredefinedAttributeObjectByFilter(const _T1& id) const {
		return FindObjectByFilter<IMetaObjectAttribute>(id, { g_metaPredefinedAttributeCLSID });
	}

#pragma endregion 

protected:

	CMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, false, eValueTypes::TYPE_BOOLEAN, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, eValueTypes::TYPE_BOOLEAN, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateBoolean(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, const bool& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, CValue(defValue), useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), false, eValueTypes::TYPE_NUMBER, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), fillCheck, eValueTypes::TYPE_NUMBER, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateNumber(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned char precision, unsigned char scale, bool fillCheck, const number_t& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierNumber(precision, scale), fillCheck, CValue(defValue), useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), false, eValueTypes::TYPE_DATE, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), fillCheck, eValueTypes::TYPE_DATE, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateDate(const wxString& name, const wxString& synonym, const wxString& comment,
		eDateFractions dateTime, bool fillCheck, const wxDateTime& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierDate(dateTime), fillCheck, CValue(defValue), useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), false, eValueTypes::TYPE_STRING, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), fillCheck, eValueTypes::TYPE_STRING, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateString(const wxString& name, const wxString& synonym, const wxString& comment,
		unsigned short length, bool fillCheck, const wxString& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, CQualifierString(length), fillCheck, CValue(defValue), useItem, selectMode);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CMetaObjectAttributePredefined* CreateEmptyType(const wxString& name, const wxString& synonym, const wxString& comment,
		eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, false, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateEmptyType(const wxString& name, const wxString& synonym, const wxString& comment,
		bool fillCheck, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, fillCheck, useItem, selectMode);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, clsid, false, CValue(), useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, clsid, false, defValue, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, bool fillCheck, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, clsid, fillCheck, defValue, useItem, selectMode);
	}

	CMetaObjectAttributePredefined* CreateSpecialType(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr,
		bool fillCheck, const CValue& defValue, eItemMode useItem = eItemMode::eItemMode_Item, eSelectMode selectMode = eSelectMode::eSelectMode_Items) {
		return IMetaObject::CreateMetaObjectAndSetParent<CMetaObjectAttributePredefined>(name, synonym, comment, clsid, descr, fillCheck, defValue, useItem, selectMode);
	}

#pragma region __array_h__

	template <typename _T1>
	bool FillArrayObjectByFilter(
		std::vector<_T1*>& array,
		std::initializer_list<class_identifier_t> filter) const
	{
		//array.reserve(array.size() + m_children.size());
		// 	
		for (auto& child : m_children) {

			if (!child->IsAllowed())
				continue;

			if (filter.size() > 0) {
				bool success = false;
				class_identifier_t child_clsid = child->GetClassType();
				for (const auto filter_clsid : filter) {
					if (child_clsid == filter_clsid) {
						success = true;
						break;
					}
				}

				if (success)
					array.emplace_back(static_cast<_T1*>(child));
			}
			else {
				_T1* ptr = dynamic_cast<_T1*>(child);
				if (ptr != nullptr) array.emplace_back(ptr);
			}
		}

		return array.size() > 0;
	}

#pragma endregion 
#pragma region __filter_h__

	template<typename MetaObject>
	MetaObject* FindObjectByFilter(const wxString& name, std::initializer_list<class_identifier_t> filter) const
	{
		for (auto& child : m_children) {

			if (child->IsDeleted())
				continue;

			if (stringUtils::CompareString(name, child->GetName())) {

				if (filter.size() > 0) {

					bool success = false;
					class_identifier_t child_clsid = child->GetClassType();
					for (const auto filter_clsid : filter) {
						if (child_clsid == filter_clsid) {
							success = true;
							break;
						}
					}

					return success ?
						static_cast<MetaObject*>(child) : nullptr;
				}

				return dynamic_cast<MetaObject*>(child);
			}
		}

		//self 
		if (stringUtils::CompareString(name, GetName()))
			return dynamic_cast<MetaObject*>(const_cast<IMetaObjectCompositeData*>(this));

		return nullptr;
	}

	template<typename MetaObject>
	MetaObject* FindObjectByFilter(const meta_identifier_t& id, std::initializer_list<class_identifier_t> filter) const
	{
		for (auto& child : m_children) {

			if (child->IsDeleted())
				continue;

			if (child->CompareId(id)) {

				if (filter.size() > 0) {

					bool success = false;
					class_identifier_t child_clsid = child->GetClassType();
					for (const auto filter_clsid : filter) {
						if (child_clsid == filter_clsid) {
							success = true;
							break;
						}
					}

					return success ?
						static_cast<MetaObject*>(child) : nullptr;
				}

				return dynamic_cast<MetaObject*>(child);
			}
		}

		//self 
		if (CompareId(id))
			return dynamic_cast<MetaObject*>(const_cast<IMetaObjectCompositeData*>(this));

		return nullptr;
	}

	template<typename MetaObject>
	MetaObject* FindObjectByFilter(const CGuid& id, std::initializer_list<class_identifier_t> filter) const
	{
		for (auto& child : m_children) {

			if (child->IsDeleted())
				continue;

			if (child->CompareGuid(id)) {

				if (filter.size() > 0) {

					bool success = false;
					class_identifier_t child_clsid = child->GetClassType();
					for (const auto filter_clsid : filter) {
						if (child_clsid == filter_clsid) {
							success = true;
							break;
						}
					}

					return success ?
						static_cast<MetaObject*>(child) : nullptr;
				}

				return dynamic_cast<MetaObject*>(child);
			}
		}

		//self 
		if (CompareGuid(id))
			return dynamic_cast<MetaObject*>(const_cast<IMetaObjectCompositeData*>(this));

		return nullptr;
	}

#pragma endregion 

	virtual bool FillArrayObjectByPredefined(
		std::vector<IMetaObjectAttribute*>& array) const {
		return false;
	}
};

#endif