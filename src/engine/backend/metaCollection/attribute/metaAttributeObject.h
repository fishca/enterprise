#ifndef __ATTRIBUTE_OBJECT_H__
#define __ATTRIBUTE_OBJECT_H__

#include "backend/metaCollection/metaObject.h"
#include "backend/backend_type.h"

#include "metaAttributeObjectEnum.h"

class BACKEND_API IValueMetaObjectAttribute :
	public IValueMetaObject, public IBackendTypeConfigFactory {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectAttribute);
public:

	enum eFieldTypes {
		eFieldTypes_Empty = 0,
		eFieldTypes_Boolean,
		eFieldTypes_Number,
		eFieldTypes_Date,
		eFieldTypes_String,
		eFieldTypes_Null,
		eFieldTypes_Enum,
		eFieldTypes_Reference,
	};

	struct sqlField_t {

		wxString m_fieldTypeName;
		struct sqlData_t {
			eFieldTypes m_type;
			struct data_t {
				wxString m_fieldName;
				struct refData_t {
					wxString m_fieldRefType;
					wxString m_fieldRefName;
					refData_t() {
					}
					refData_t(const wxString& fieldRefType, const wxString& fieldRefName)
						: m_fieldRefType(fieldRefType), m_fieldRefName(fieldRefName) {
					}
					~refData_t() {
					}
				} m_fieldRefName;

				data_t()
					: m_fieldName(wxEmptyString)
				{
				}

				data_t(const wxString& fieldName)
					: m_fieldName(fieldName) {
				}

				data_t(const wxString& fieldRefType, const wxString& fieldRefNam)
					: m_fieldRefName(fieldRefType, fieldRefNam) {
				}

				~data_t() {
				}

			} m_field;

			sqlData_t() : m_type(eFieldTypes::eFieldTypes_Empty)
			{
			}
			sqlData_t(eFieldTypes type) : m_type(type)
			{
			}
			sqlData_t(eFieldTypes type, const wxString& fieldName) : m_type(type), m_field(fieldName)
			{
			}
			sqlData_t(eFieldTypes type, const wxString& fieldRefType, const wxString& fieldRefName) : m_type(type), m_field(fieldRefType, fieldRefName)
			{
			}
			sqlData_t(const sqlData_t& rhs) : m_type(rhs.m_type)
			{
				if (rhs.m_type != eFieldTypes::eFieldTypes_Reference) {
					m_field.m_fieldName = rhs.m_field.m_fieldName;
				}
				else {
					m_field.m_fieldRefName.m_fieldRefType = rhs.m_field.m_fieldRefName.m_fieldRefType;
					m_field.m_fieldRefName.m_fieldRefName = rhs.m_field.m_fieldRefName.m_fieldRefName;
				}
			}
			sqlData_t& operator=(const sqlData_t& rhs) {
				m_type = rhs.m_type;
				if (rhs.m_type != eFieldTypes::eFieldTypes_Reference) {
					m_field.m_fieldName = rhs.m_field.m_fieldName;
				}
				else {
					m_field.m_fieldRefName.m_fieldRefType = rhs.m_field.m_fieldRefName.m_fieldRefType;
					m_field.m_fieldRefName.m_fieldRefName = rhs.m_field.m_fieldRefName.m_fieldRefName;
				}
				return *this;
			}
			~sqlData_t() {}
		};

		std::vector< sqlData_t> m_types;

		sqlField_t(const wxString& fieldTypeName) : m_fieldTypeName(fieldTypeName) {
		}

		void AppendType(eFieldTypes type) {
			m_types.emplace_back(type);
		}

		void AppendType(eFieldTypes type, const wxString& fieldName) {
			m_types.emplace_back(type, fieldName);
		}

		void AppendType(eFieldTypes type, const wxString& fieldRefType, const wxString& fieldRefName) {
			m_types.emplace_back(type, fieldRefType, fieldRefName);
		}

		///////////////////////////////////////////////////////
		auto begin() { return m_types.begin(); }
		auto end() { return m_types.end(); }
		///////////////////////////////////////////////////////
	};

	//get special filed data
	static unsigned short GetSQLFieldCount(const IValueMetaObjectAttribute* metaAttr);
	static wxString GetSQLFieldName(const IValueMetaObjectAttribute* metaAttr, const wxString& aggr = wxEmptyString);
	static wxString GetCompositeSQLFieldName(const IValueMetaObjectAttribute* metaAttr, const wxString& cmp = wxT("="));
	static wxString GetExcludeSQLFieldName(const IValueMetaObjectAttribute* metaAttr);

	//get data sql
	static sqlField_t GetSQLFieldData(const IValueMetaObjectAttribute* metaAttr);

	//process default query
	static int ProcessAttribute(const wxString& tableName, const IValueMetaObjectAttribute* srcAttr, const IValueMetaObjectAttribute* dstAttr);

	//set value attribute 
	static void SetValueAttribute(const IValueMetaObjectAttribute* attribute, const CValue& cValue, class IPreparedStatement* statement, int& position);
	static void SetValueAttribute(const IValueMetaObjectAttribute* attribute, const CValue& cValue, class IPreparedStatement* statement);

	//get value from attribute
	static bool GetValueAttribute(const wxString& fieldName, const eFieldTypes& fldType, const IValueMetaObjectAttribute* metaAttr, CValue& retValue, class IDatabaseResultSet* resultSet, bool createData = true);
	static bool GetValueAttribute(const wxString& fieldName, const IValueMetaObjectAttribute* attribute, CValue& retValue, class IDatabaseResultSet* resultSet, bool createData = true);
	static bool GetValueAttribute(const IValueMetaObjectAttribute* attribute, CValue& retValue, class IDatabaseResultSet* resultSet, bool createData = true);

	//store value 
	static void SetBinaryData(const IValueMetaObjectAttribute* metaAttr, const CMemoryReader& reader, IPreparedStatement* statement,
		int& position);
	static void SetBinaryData(const IValueMetaObjectAttribute* metaAttr, const CMemoryReader& reader, IPreparedStatement* statement);
	static void GetBinaryData(const IValueMetaObjectAttribute* metaAttr, CMemoryWriter& writer, IDatabaseResultSet* resultSet);

	//contain type
	bool ContainType(const eValueTypes& valType) const;
	bool ContainType(const class_identifier_t& clsid) const;

	//contain meta type
	bool ContainMetaType(enum eCtorMetaType type) const;

	//equal type 
	bool EqualType(const class_identifier_t& clsid, const CTypeDescription& rhs) const;

	//ctor 
	IValueMetaObjectAttribute(const wxString& name = wxEmptyString, const wxString& synonym = wxEmptyString, const wxString& comment = wxEmptyString) :
		IValueMetaObject(name, synonym, comment)
	{
	}

#pragma region value_factory 

	//get data selector 
	virtual eSelectorDataType GetFilterDataType() const;

	//Create value by selected type
	virtual CValue CreateValue() const;
	virtual CValue* CreateValueRef() const;

#pragma endregion

	virtual wxString GetFieldNameDB() const { return wxString::Format(wxT("fld%i"), m_metaId); }

	//get sql type for db 
	virtual wxString GetSQLTypeObject(const class_identifier_t& clsid) const;

	//check if attribute is fill 
	virtual bool FillCheck() const = 0;

	virtual eItemMode GetItemMode() const { return eItemMode::eItemMode_Item; }
	virtual eSelectMode GetSelectMode() const { return eSelectMode::eSelectMode_Items; }

	//get metaData
	virtual IMetaData* GetMetaData() const { return m_metaData; }

	//events:
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnDeleteMetaObject();

	//for designer 
	virtual bool OnReloadMetaObject();

	//module manager is started or exit 
	// //after and before for designer 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterRunMetaObject(int flags);

protected:
	CValue m_defValue;
};

class BACKEND_API CValueMetaObjectAttribute : public IValueMetaObjectAttribute {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectAttribute);
public:

	CValueMetaObjectAttribute::CValueMetaObjectAttribute(const eValueTypes& valType = eValueTypes::TYPE_STRING) :
		IValueMetaObjectAttribute()
	{
		m_propertyType->SetValue(CValue::GetIDByVT(valType));
	}

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//check if attribute is fill 
	virtual bool FillCheck() const { return m_propertyFillCheck->GetValueAsBoolean() && GetClsidCount() > 0; }

	virtual eItemMode GetItemMode() const;
	virtual eSelectMode GetSelectMode() const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const { return m_propertyType->GetValueAsTypeDesc(); }

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	CPropertyCategory* m_categoryType = IPropertyObject::CreatePropertyCategory(wxT("Data"), _("Data"));
	CPropertyType* m_propertyType = IPropertyObject::CreateProperty<CPropertyType>(m_categoryType, wxT("Type"), _("Type"), eValueTypes::TYPE_STRING);
	CPropertyCategory* m_categoryAttribute = IPropertyObject::CreatePropertyCategory(wxT("Attribute"), _("Attribute"));
	CPropertyBoolean* m_propertyFillCheck = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryAttribute, wxT("FillCheck"), _("Fill check"));
	CPropertyCategory* m_categoryPresentation = IPropertyObject::CreatePropertyCategory(wxT("Presentation"), _("Presentation"));
	CPropertyEnum<CValueEnumSelectMode>* m_propertySelectMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumSelectMode>>(m_categoryPresentation, wxT("Select"), _("Select group and items"), eSelectMode::eSelectMode_Items);
	CPropertyCategory* m_categoryGroup = IPropertyObject::CreatePropertyCategory(wxT("Group"), _("Group"));
	CPropertyEnum<CValueEnumItemMode>* m_propertyItemMode = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumItemMode>>(m_categoryGroup, wxT("ItemMode"), _("Item mode"), eItemMode::eItemMode_Item);
};

class BACKEND_API CValueMetaObjectAttributePredefined : public IValueMetaObjectAttribute {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectAttribute);
private:

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(eValueTypes::TYPE_BOOLEAN);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment, const CQualifierNumber& qNumber, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(eValueTypes::TYPE_NUMBER);
		m_typeDesc.SetNumber(qNumber.m_precision, qNumber.m_scale);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment, const CQualifierDate& qDate, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(eValueTypes::TYPE_DATE);
		m_typeDesc.SetDate(qDate.m_dateTime);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment, const CQualifierString& qString, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(eValueTypes::TYPE_STRING);
		m_typeDesc.SetString(qString.m_length);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(clsid);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment,
		const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr, bool fillCheck, const CValue& defValue, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.SetDefaultMetaType(clsid, descr);
		m_fillCheck = fillCheck; m_defValue = defValue;
	}

	CValueMetaObjectAttributePredefined(const wxString& name, const wxString& synonym, const wxString& comment, bool fillCheck, eItemMode itemMode, eSelectMode selectMode)
		: IValueMetaObjectAttribute(name, synonym, comment), m_itemMode(itemMode), m_selectMode(selectMode)
	{
		m_typeDesc.ClearMetaType();
		m_fillCheck = fillCheck;
	}

public:

	CValueMetaObjectAttributePredefined()
		: IValueMetaObjectAttribute(), m_itemMode(eItemMode::eItemMode_Item), m_selectMode(eSelectMode::eSelectMode_Items) {
		m_typeDesc.SetDefaultMetaType(eValueTypes::TYPE_STRING);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//check if attribute is fill 
	virtual bool FillCheck() const { return m_fillCheck && m_typeDesc.GetClsidCount() > 0; }
	virtual eItemMode GetItemMode() const { return m_itemMode; }
	virtual eSelectMode GetSelectMode() const { return m_selectMode; }

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const { return m_typeDesc; }

	friend class CValue;

protected:

	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:

	mutable CTypeDescription m_typeDesc;
	bool m_fillCheck;
	eItemMode m_itemMode;
	eSelectMode m_selectMode;
};

#endif