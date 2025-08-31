#ifndef __VALUE_TABLE_H__
#define __VALUE_TABLE_H__

#include "valueArray.h"
#include "valueMap.h"

#include "backend/tableInfo.h"

const class_identifier_t g_valueTableCLSID = string_to_clsid("VL_TABL");

//Table support
class BACKEND_API CValueTable : public IValueTable {
	wxDECLARE_DYNAMIC_CLASS(CValueTable);
private:
	// methods:
	enum Func {
		enAddRow = 0,
		enClone,
		enCount,
		enFind,
		enDelete,
		enClear,
		enSort,
	};
	//attributes:
	enum Prop {
		enColumns = 0,
	};
public:
	class CValueTableColumnCollection : public IValueTable::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CValueTableColumnCollection);
	private:
		enum Func {
			enAddColumn = 0,
			enRemoveColumn
		};
	public:

		class CValueTableColumnInfo : public IValueTable::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CValueTableColumnInfo);
		private:

			unsigned int m_columnID;
			wxString m_columnName;
			CTypeDescription m_columnType;
			wxString m_columnCaption;
			int m_columnWidth;

		public:

			CValueTableColumnInfo();
			CValueTableColumnInfo(unsigned int colId, const wxString& colName, const CTypeDescription& typeDescription, const wxString& caption, int width);
			virtual ~CValueTableColumnInfo();

			virtual unsigned int GetColumnID() const { return m_columnID; }
			virtual void SetColumnID(unsigned int col) { m_columnID = col; }
			virtual wxString GetColumnName() const { return m_columnName; }
			virtual void SetColumnName(const wxString& name) { m_columnName = name; }
			virtual wxString GetColumnCaption() const { return m_columnCaption; }
			virtual void SetColumnCaption(const wxString& caption) { m_columnCaption = caption; }
			virtual const CTypeDescription GetColumnType() const { return m_columnType; }
			virtual void SetColumnType(const CTypeDescription& typeDescription) { m_columnType = typeDescription; }
			virtual int GetColumnWidth() const { return m_columnWidth; }
			virtual void SetColumnWidth(int width) { m_columnWidth = width; }

			friend CValueTableColumnCollection;
		};

	public:

		CValueTableColumnCollection(CValueTable* ownerTable = nullptr);
		virtual ~CValueTableColumnCollection();

		IValueModelColumnInfo* AddColumn(const wxString& colName,
			const CTypeDescription& typeData,
			const wxString& caption,
			int width = wxDVC_DEFAULT_WIDTH) override {

			unsigned int max_id = 0;

			for (auto& col : m_listColumnInfo) {
				if (max_id < col->GetColumnID()) {
					max_id = col->GetColumnID();
				}
			}

			for (long row = 0; row < m_ownerTable->GetRowCount(); row++) {
				wxValueTableRow* node = m_ownerTable->GetViewData<wxValueTableRow>(m_ownerTable->GetItem(row));
				wxASSERT(node);
				node->SetValue(max_id + 1, CValueTypeDescription::AdjustValue(typeData));
			}

			return m_listColumnInfo.emplace_back(new CValueTableColumnInfo(max_id + 1, colName, typeData, caption, width));
		}

		const CTypeDescription GetColumnType(unsigned int col) const {
			for (auto& colInfo : m_listColumnInfo) {
				if (col == colInfo->GetColumnID()) {
					return colInfo->GetColumnType();
				}
			}
			return CTypeDescription();
		}

		virtual void RemoveColumn(unsigned int col) {

			for (long row = 0; row < m_ownerTable->GetRowCount(); row++) {
				wxValueTableRow* node = m_ownerTable->GetViewData<wxValueTableRow>(m_ownerTable->GetItem(row));
				wxASSERT(node);
				node->EraseValue(col);
			}

			auto& it = std::find_if(m_listColumnInfo.begin(), m_listColumnInfo.end(),
				[col](CValueTableColumnInfo* colInfo) {
					return col == colInfo->GetColumnID();
				}
			);

			m_listColumnInfo.erase(it);
		}

		virtual IValueModelColumnInfo* GetColumnInfo(unsigned int idx) const {
			if (m_listColumnInfo.size() < idx)
				return nullptr;
			auto& it = m_listColumnInfo.begin();
			std::advance(it, idx);
			return *it;
		}

		virtual unsigned int GetColumnCount() const { return m_listColumnInfo.size(); }

		virtual CMethodHelper* GetPMethods() const {
			//PrepareNames();
			return m_methodHelper;
		}

		virtual void PrepareNames() const;


		//WORK AS AN AGGREGATE OBJECT
		virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);
		virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

		friend class CValueTable;

	protected:

		CValueTable* m_ownerTable;
		std::vector<CValuePtr<CValueTableColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;

	};

	class CValueTableReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CValueTableReturnLine);
	public:
		CValueTable* m_ownerTable;
	public:

		CValueTableReturnLine(CValueTable* ownerTable = nullptr, const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CValueTableReturnLine();

		virtual IValueTable* GetOwnerModel() const { return m_ownerTable; }

		virtual CMethodHelper* GetPMethods() const {
			//PrepareNames();
			return m_methodHelper;
		}

		virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

	private:
		CMethodHelper* m_methodHelper;
	};

public:

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	virtual bool AutoCreateColumn() const { return true; }

	virtual IValueModelColumnCollection* GetColumnCollection() const { return m_tableColumnCollection; }

	virtual CValueTableReturnLine* GetRowAt(const long& line) {
		if (line > GetRowCount())
			return nullptr;
		return CValue::CreateAndConvertObjectValueRef<CValueTableReturnLine>(this, GetItem(line));
	}

	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) {
		if (!line.IsOk())
			return nullptr;
		return GetRowAt(GetRow(line));
	}

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal) {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node == nullptr)
			return false;
		return node->SetValue(id, CValueTypeDescription::AdjustValue(m_tableColumnCollection->GetColumnType(id), varMetaVal), true);
	}

	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node == nullptr)
			return false;
		return node->GetValue(id, pvarMetaVal);
	}

	CValueTable();
	CValueTable(const CValueTable& val);
	virtual ~CValueTable();

	virtual void AddValue(unsigned int before = 0) {
		long row = GetRow(GetSelection());
		if (row > 0)
			AppendRow(row);
		else AppendRow();
	}

	virtual void CopyValue() { CopyRow(); }
	virtual void EditValue() { EditRow(); }
	virtual void DeleteValue() { DeleteRow(); }

	//array
	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	//check is empty
	virtual inline bool IsEmpty() const { return GetRowCount() == 0; }

	virtual CMethodHelper* GetPMethods() const {  // get a reference to the class helper for parsing attribute and method names
		//PrepareNames();
		return &m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); // attribute value
	virtual bool CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray);       // method call

	// implementation of base class virtuals to define model
	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const override;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	//support def. methods (in runtime)
	long AppendRow(unsigned int before = 0);
	void CopyRow();
	void EditRow();
	void DeleteRow();

	CValueTable* Clone() { return CValue::CreateAndConvertObjectValueRef<CValueTable>(*this); }
	unsigned int Count() { return GetRowCount(); }
	void Clear();

#pragma region _tabular_data_
	//get metaData from object 
	virtual IMetaObjectSourceData* GetSourceMetaObject() const { return nullptr; }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const { return g_valueTableCLSID; }
#pragma endregion 

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//Working with iterators
	virtual bool HasIterator() const override { return true; }
	virtual CValue GetIteratorEmpty() override {
		return CValue::CreateAndConvertObjectValueRef <CValueTableReturnLine>(this, wxDataViewItem(nullptr));
	}
	virtual CValue GetIteratorAt(unsigned int idx) override {
		if (idx > (unsigned int)GetRowCount())
			return CValue();
		return CValue::CreateAndConvertObjectValueRef <CValueTableReturnLine>(this, GetItem(idx));
	}

	virtual unsigned int GetIteratorCount() const override { return GetRowCount(); }

private:

	CValuePtr<CValueTableColumnCollection> m_tableColumnCollection;
	static CMethodHelper m_methodHelper;
};

#endif