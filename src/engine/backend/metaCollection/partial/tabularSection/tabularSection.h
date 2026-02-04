#ifndef _VALUETABLEPART_H__

#include "backend/tableInfo.h"
#include "backend/valueInfo.h"

#include "backend/metaCollection/table/metaTableObject.h"

class BACKEND_API IValueTabularSectionDataObject : public IValueTable {
	wxDECLARE_ABSTRACT_CLASS(IValueTabularSectionDataObject);
private:

	enum Func {
		enAddValue = 0,
		enCount,
		enFind,
		enDelete,
		enClear,
		enLoad,
		enUnload,
		enGetMetadata,
	};

	enum {
		eTabularSection,
	};

public:

	virtual IValueModelColumnCollection* GetColumnCollection() const override { return m_recordColumnCollection; }
	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) {
		if (!line.IsOk())
			return nullptr;
		return CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectReturnLine>(this, line);
	}

	virtual bool HasDefaultCompare() const override { return false; }

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const;

	virtual CValue GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id) const {
		CValue retValue;
		if (GetValueByMetaID(item, id, retValue))
			return retValue;
		return CValue();
	}

	class CValueTabularSectionDataObjectColumnCollection : public IValueTable::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CValueTabularSectionDataObjectColumnCollection);
	public:
		class CValueTabularSectionColumnInfo : public IValueTable::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CValueTabularSectionColumnInfo);
		public:

			virtual unsigned int GetColumnID() const { return m_metaAttribute->GetMetaID(); }
			virtual wxString GetColumnName() const { return m_metaAttribute->GetName(); }
			virtual wxString GetColumnCaption() const { return m_metaAttribute->GetSynonym(); }
			virtual const CTypeDescription GetColumnType() const { return m_metaAttribute->GetTypeDesc(); }

			CValueTabularSectionColumnInfo();
			CValueTabularSectionColumnInfo(IValueMetaObjectAttribute* attribute);
			virtual ~CValueTabularSectionColumnInfo();

		private:
			IValueMetaObjectAttribute* m_metaAttribute;
			friend CValueTabularSectionDataObjectColumnCollection;
		};

	public:

		CValueTabularSectionDataObjectColumnCollection();
		CValueTabularSectionDataObjectColumnCollection(IValueTabularSectionDataObject* ownerTable);
		virtual ~CValueTabularSectionDataObjectColumnCollection();

		virtual const CTypeDescription GetColumnType(unsigned int col) const {
			return m_listColumnInfo.at(col)->GetColumnType();
		}

		virtual IValueModelColumnInfo* GetColumnInfo(unsigned int idx) const {
			if (m_listColumnInfo.size() < idx)
				return nullptr;
			auto& it = m_listColumnInfo.begin();
			std::advance(it, idx);
			return it->second;
		}

		virtual unsigned int GetColumnCount() const { return m_listColumnInfo.size(); }

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		friend class IValueTabularSectionDataObject;

	protected:

		IValueTabularSectionDataObject* m_ownerTable;
		std::map<meta_identifier_t, CValuePtr<CValueTabularSectionColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;
	};

	class CValueTabularSectionDataObjectReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CValueTabularSectionDataObjectReturnLine);
	public:

		CValueTabularSectionDataObjectReturnLine(IValueTabularSectionDataObject* ownerTable = nullptr, const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CValueTabularSectionDataObjectReturnLine();

		virtual IValueTable* GetOwnerModel() const { return m_ownerTable; }
		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

		//Get ref class 
		virtual class_identifier_t GetClassType() const;

		virtual wxString GetClassName() const;
		virtual wxString GetString() const;

		friend class IValueTabularSectionDataObject;
	private:
		IValueTabularSectionDataObject* m_ownerTable;
		CMethodHelper* m_methodHelper;
	};

	CValueMetaObjectTableData* GetMetaObject() const { return m_metaTable; }
	meta_identifier_t GetMetaID() const { return m_metaTable ? m_metaTable->GetMetaID() : wxNOT_FOUND; }

#pragma region _source_data_

	//get metaData from object 
	virtual IValueMetaObjectCompositeData* GetSourceMetaObject() const { return m_metaTable; }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const { return GetClassType(); }

#pragma endregion 

	IValueTabularSectionDataObject() :
		m_objectValue(nullptr), m_metaTable(nullptr),
		m_recordColumnCollection(nullptr),
		m_methodHelper(nullptr), m_readOnly(false) {
	}

	IValueTabularSectionDataObject(IValueDataObject* objectValue, CValueMetaObjectTableData* tableObject, bool readOnly = false) :
		m_objectValue(objectValue), m_metaTable(tableObject),
		m_recordColumnCollection(CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectColumnCollection>(this)),
		m_methodHelper(new CMethodHelper()), m_readOnly(readOnly) {
		for (const auto object : tableObject->GetGenericAttributeArrayObject()) {
			m_filterRow.AppendFilter(
				object->GetMetaID(),
				object->GetName(),
				object->GetSynonym(),
				eComparisonType_Equal,
				object->GetTypeDesc(),
				object->CreateValue(),
				false
			);
		}
	}

	virtual ~IValueTabularSectionDataObject() { wxDELETE(m_methodHelper); }

	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const override;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	virtual bool AutoCreateColumn() const { return false; }
	virtual bool EditableLine(const wxDataViewItem& item, unsigned int col) const {
		return !m_metaTable->IsNumberLine(col);
	}

	virtual void ActivateItem(IBackendValueForm* formOwner,
		const wxDataViewItem& item, unsigned int col) {
		IValueTable::RowValueStartEdit(item, col);
	}

	virtual void AddValue(unsigned int before = 0);
	virtual void CopyValue();
	virtual void EditValue();
	virtual void DeleteValue();

	//append new row
	virtual long AppendRow(unsigned int before = 0);

	virtual bool LoadData(const CGuid& srcGuid, bool createData = true) { return true; }
	virtual bool SaveData() { return true; }
	virtual bool DeleteData() { return true; }

	virtual bool LoadDataFromTable(IValueTable* srcTable);
	virtual IValueTable* SaveDataToTable() const;

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************

	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;                             // this method is automatically called to initialize attribute and method names
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       // method call

	//array
	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//Working with iterators
	virtual bool HasIterator() const override { return true; }

	virtual CValue GetIteratorEmpty() override {
		return CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectReturnLine>(this, wxDataViewItem(nullptr));
	}

	virtual CValue GetIteratorAt(unsigned int idx) override {
		if (idx > (unsigned int)GetRowCount())
			return wxEmptyValue;
		return CValue::CreateAndPrepareValueRef<CValueTabularSectionDataObjectReturnLine>(this, GetItem(idx));
	}

	virtual unsigned int GetIteratorCount() const override { return GetRowCount(); }

protected:

	void RefreshTabularSection();

	virtual void RefreshModel(const wxDataViewItem& topItem = wxDataViewItem(nullptr),
		const int countPerPage = defaultCountPerPage)
	{
		RefreshTabularSection();
	}

	bool m_readOnly;

	CValueMetaObjectTableData* m_metaTable;

	IValueDataObject* m_objectValue;
	CValuePtr<CValueTabularSectionDataObjectColumnCollection> m_recordColumnCollection;
	CMethodHelper* m_methodHelper;
};

class BACKEND_API CValueTabularSectionDataObject : public IValueTabularSectionDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueTabularSectionDataObject);
public:

	CValueTabularSectionDataObject();
	CValueTabularSectionDataObject(class IValueRecordDataObject* recordObject, CValueMetaObjectTableData* tableObject);
	virtual ~CValueTabularSectionDataObject() {}
};

class BACKEND_API CValueTabularSectionDataObjectRef : public IValueTabularSectionDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueTabularSectionDataObjectRef);
public:

	bool IsReadAfter() const { return m_readAfter; }

	CValueTabularSectionDataObjectRef();
	CValueTabularSectionDataObjectRef(class CValueReferenceDataObject* reference, CValueMetaObjectTableData* tableObject, bool readAfter = false);
	CValueTabularSectionDataObjectRef(class IValueRecordDataObjectRef* recordObject, CValueMetaObjectTableData* tableObject);
	CValueTabularSectionDataObjectRef(class CValueSelectorRecordDataObject* selectorObject, CValueMetaObjectTableData* tableObject);

	virtual ~CValueTabularSectionDataObjectRef() {}

	virtual void CopyValue();
	virtual void DeleteValue();

	//append new row
	virtual long AppendRow(unsigned int before = 0);

	//load/save/delete data
	virtual bool LoadData(const CGuid& srcGuid, bool createData = true);
	virtual bool SaveData();
	virtual bool DeleteData();

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal);
	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const;

protected:
	bool m_readAfter;
};

#endif // !_VALUEUUID_H__
