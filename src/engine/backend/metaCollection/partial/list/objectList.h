#ifndef _OBJECT_LIST_H__
#define _OBJECT_LIST_H__

#include "backend/metaCollection/partial/commonObject.h"
#include "backend/metaCollection/partial/reference/reference.h"

//base list class 
class BACKEND_API IValueListDataObject : public IValueTable,
	public ISourceDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueListDataObject);
protected:
	enum Func {
		enRefresh
	};
private:

	// implementation of base class virtuals to define model
	virtual IValueModelColumnCollection* GetColumnCollection() const override { return m_recordColumnCollection; }
	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) override {
		if (!line.IsOk())
			return nullptr;
		return CValue::CreateAndPrepareValueRef<CValueDataObjectListReturnLine>(this, line);
	}

public:

	virtual int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
		unsigned int col, bool ascending) const override {
		wxASSERT(item1.IsOk() && item2.IsOk());
		const int long row1 = GetRow(item1);
		const int long row2 = GetRow(item2);
		if (row1 < row2)
			return -1;
		else if (row1 > row2)
			return 1;
		return 0;
	}

	class CValueDataObjectListColumnCollection : public IValueTable::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CValueDataObjectListColumnCollection);
	public:
		class CDataObjectListColumnInfo : public IValueTable::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CDataObjectListColumnInfo);
		public:

			virtual unsigned int GetColumnID() const { return m_metaAttribute->GetMetaID(); }
			virtual wxString GetColumnName() const { return m_metaAttribute->GetName(); }
			virtual wxString GetColumnCaption() const { return m_metaAttribute->GetSynonym(); }
			virtual const CTypeDescription GetColumnType() const { return m_metaAttribute->GetTypeDesc(); }

			CDataObjectListColumnInfo();
			CDataObjectListColumnInfo(IValueMetaObjectAttribute* attribute);
			virtual ~CDataObjectListColumnInfo();

		private:
			IValueMetaObjectAttribute* m_metaAttribute;
		};

	public:

		CValueDataObjectListColumnCollection();
		CValueDataObjectListColumnCollection(IValueListDataObject* ownerTable, IValueMetaObjectGenericData* metaObject);
		virtual ~CValueDataObjectListColumnCollection();

		virtual const CTypeDescription GetColumnType(unsigned int col) const {
			CDataObjectListColumnInfo* columnInfo = m_listColumnInfo.at(col);
			wxASSERT(columnInfo);
			return columnInfo->GetColumnType();
		}

		virtual IValueModelColumnInfo* GetColumnInfo(unsigned int idx) const {
			if (m_listColumnInfo.size() < idx)
				return nullptr;
			auto& it = m_listColumnInfo.begin();
			std::advance(it, idx);
			return it->second;
		}

		virtual unsigned int GetColumnCount() const {
			IValueMetaObjectGenericData* metaTable = m_ownerTable->GetMetaObject();
			wxASSERT(metaTable);
			const auto& obj = metaTable->GetGenericAttributeArrayObject();
			return obj.size();
		}

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	protected:

		IValueListDataObject* m_ownerTable;
		std::map<meta_identifier_t, CValuePtr<CDataObjectListColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;
	};

	class CValueDataObjectListReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CValueDataObjectListReturnLine);
	public:

		CValueDataObjectListReturnLine(IValueListDataObject* ownerTable = nullptr, const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CValueDataObjectListReturnLine();

		virtual IValueTable* GetOwnerModel() const {
			return m_ownerTable;
		}

		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

	protected:
		IValueListDataObject* m_ownerTable;
		CMethodHelper* m_methodHelper;
	};

public:

	void AppendSort(IValueMetaObject* metaObject, bool ascending = true, bool use = true, bool system = false) {
		if (metaObject == nullptr)
			return;
		m_sortOrder.AppendSort(
			metaObject->GetMetaID(),
			metaObject->GetName(),
			metaObject->GetSynonym(),
			ascending, use, system
		);
	}

	virtual bool AutoCreateColumn() const { return false; }
	virtual bool EditableLine(const wxDataViewItem& item, unsigned int col) const { return false; }

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal) { return false; }
	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const {
		wxValueTableRow* node = GetViewData<wxValueTableRow>(item);
		if (node == nullptr)
			return false;
		return node->GetValue(id, pvarMetaVal);
	}

	virtual bool GetValueAttribute(IValueMetaObjectAttribute* metaAttr, CValue& retValue, class IDatabaseResultSet* resultSet, bool createData = true) {
		return IValueMetaObjectAttribute::GetValueAttribute(metaAttr, retValue, resultSet, createData);
	}

	//ctor
	IValueListDataObject(IValueMetaObjectGenericData* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false);
	virtual ~IValueListDataObject();

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return GetMetaObject() ?
			stringUtils::GenerateSynonym(GetMetaObject()->GetClassName()) + wxT(": ") + GetMetaObject()->GetSynonym() : GetString();
	}

	//Is new object? 
	virtual bool IsNewObject() const { return false; }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_objGuid; };

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetMetaObject() const = 0;

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	virtual bool IsEmpty() const { return false; }

	//Get ref class 
	virtual class_identifier_t GetClassType() const = 0;

	virtual wxString GetClassName() const = 0;
	virtual wxString GetString() const = 0;

protected:
	CGuid m_objGuid;
	CValuePtr<CValueDataObjectListColumnCollection> m_recordColumnCollection;
	CMethodHelper* m_methodHelper;
};

// list enumeration 
class BACKEND_API CValueListDataObjectEnumRef : public IValueListDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueListDataObjectRef);
public:
	struct wxValueTableEnumRow : public wxValueTableRow {
		wxValueTableEnumRow(const CGuid& guid) :
			wxValueTableRow(), m_objGuid(guid) {
		}
		CGuid GetGuid() const {
			return m_objGuid;
		}
	private:
		CGuid m_objGuid;
	};
public:

	virtual bool UseStandartCommand() const {
		return false;
	}

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	//Constructor
	CValueListDataObjectEnumRef(IValueMetaObjectRecordDataEnumRef* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false);

	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       // method call

	//on activate item
	virtual void ActivateItem(IBackendValueForm* srcForm,
		const wxDataViewItem& item, unsigned int col) {
		if (m_choiceMode)
			ChooseValue(srcForm);
	}

	//get metaData from object 
	virtual IValueMetaObjectRecordDataEnumRef* GetMetaObject() const {
		return m_metaObject;
	};

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//events:
	virtual void ChooseValue(IBackendValueForm* srcForm);

private:

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	virtual void RefreshModel(const wxDataViewItem& topItem = wxDataViewItem(nullptr), const int countPerPage = defaultCountPerPage);
	virtual void RefreshItemModel(
		const wxDataViewItem& topItem,
		const wxDataViewItem& currentItem,
		const int countPerPage,
		const short scroll = 0
	);

private:

	bool m_choiceMode;
	IValueMetaObjectRecordDataEnumRef* m_metaObject;
};

// list without parent  
class BACKEND_API CValueListDataObjectRef : public IValueListDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueListDataObjectRef);
public:
	struct wxValueTableListRow : public wxValueTableRow {
		wxValueTableListRow(const CGuid& guid) :
			wxValueTableRow(), m_objGuid(guid) {
		}
		CGuid GetGuid() const {
			return m_objGuid;
		}
	private:
		CGuid m_objGuid;
	};
public:

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	//Constructor
	CValueListDataObjectRef(IValueMetaObjectRecordDataMutableRef* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false);

	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       // method call

	//on activate item
	virtual void ActivateItem(IBackendValueForm* srcForm,
		const wxDataViewItem& item, unsigned int col) {
		if (m_choiceMode) {
			ChooseValue(srcForm);
		}
		else {
			EditValue();
		}
	}

	//get metaData from object 
	virtual IValueMetaObjectRecordDataRef* GetMetaObject() const { return m_metaObject; }

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//events:
	virtual void AddValue(unsigned int before = 0) override;
	virtual void CopyValue() override;
	virtual void EditValue() override;
	virtual void DeleteValue() override;

	virtual void MarkAsDeleteValue();
	virtual void ChooseValue(IBackendValueForm* srcForm);

private:

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	virtual void RefreshModel(const wxDataViewItem& topItem = wxDataViewItem(nullptr), const int countPerPage = defaultCountPerPage);
	virtual void RefreshItemModel(
		const wxDataViewItem& topItem,
		const wxDataViewItem& currentItem,
		const int countPerPage,
		const short scroll = 0
	);

private:

	bool m_choiceMode;

	IValueMetaObjectRecordDataMutableRef* m_metaObject;
};

// list register
class BACKEND_API CValueListRegisterObject : public IValueListDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueListRegisterObject);
public:
	struct wxValueTableKeyRow : public wxValueTableRow {
		wxValueTableKeyRow() :
			wxValueTableRow(), m_nodeKeys() {
		}
		void AppendNodeValue(const meta_identifier_t& id, const CValue& variant) { m_nodeKeys.insert_or_assign(id, variant); }
		CValue& AppendNodeValue(const meta_identifier_t& id) { return m_nodeKeys[id]; }
		CUniquePairKey GetUniquePairKey(IValueMetaObjectRegisterData* metaObject) const { return CUniquePairKey(metaObject, m_nodeValues); }

	private:
		valueArray_t m_nodeKeys;
	};
public:

	virtual bool UseStandartCommand() const { return !m_metaObject->HasRecorder(); }

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	//Constructor
	CValueListRegisterObject(IValueMetaObjectRegisterData* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND);

	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) const;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& row, unsigned int col) override;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}

	virtual void PrepareNames() const;
	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);       // method call

	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//on activate item
	virtual void ActivateItem(IBackendValueForm* srcForm,
		const wxDataViewItem& item, unsigned int col) {
		EditValue();
	}

	//get metaData from object 
	virtual IValueMetaObjectRegisterData* GetMetaObject() const {
		return m_metaObject;
	};

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//events:
	virtual void AddValue(unsigned int before = 0) override;
	virtual void CopyValue() override;
	virtual void EditValue() override;
	virtual void DeleteValue() override;

private:

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	virtual void RefreshModel(const wxDataViewItem& topItem = wxDataViewItem(nullptr), const int countPerPage = defaultCountPerPage);
	virtual void RefreshItemModel(
		const wxDataViewItem& topItem,
		const wxDataViewItem& currentItem,
		const int countPerPage,
		const short scroll = 0
	);

private:
	IValueMetaObjectRegisterData* m_metaObject;
};

//base tree class 
class BACKEND_API IValueTreeDataObject : public IValueTree,
	public ISourceDataObject {
	wxDECLARE_ABSTRACT_CLASS(IValueTreeDataObject);
protected:
	enum Func {
		enRefresh
	};
private:

	// implementation of base class virtuals to define model
	virtual IValueModelColumnCollection* GetColumnCollection() const override { return m_recordColumnCollection; }
	virtual IValueModelReturnLine* GetRowAt(const wxDataViewItem& line) {
		if (!line.IsOk())
			return nullptr;
		return CValue::CreateAndPrepareValueRef<CValueDataObjectTreeReturnLine>(this, line);
	}

public:

	class CValueDataObjectTreeColumnCollection : public IValueTree::IValueModelColumnCollection {
		wxDECLARE_DYNAMIC_CLASS(CValueDataObjectTreeColumnCollection);
	public:
		class CDataObjectTreeColumnInfo : public IValueTree::IValueModelColumnCollection::IValueModelColumnInfo {
			wxDECLARE_DYNAMIC_CLASS(CDataObjectTreeColumnInfo);
		public:

			virtual unsigned int GetColumnID() const { return m_metaAttribute->GetMetaID(); }
			virtual wxString GetColumnName() const { return m_metaAttribute->GetName(); }
			virtual wxString GetColumnCaption() const { return m_metaAttribute->GetSynonym(); }
			virtual const CTypeDescription GetColumnType() const { return m_metaAttribute->GetTypeDesc(); }

			CDataObjectTreeColumnInfo();
			CDataObjectTreeColumnInfo(IValueMetaObjectAttribute* attribute);
			virtual ~CDataObjectTreeColumnInfo();

		private:
			IValueMetaObjectAttribute* m_metaAttribute;
		};

	public:

		CValueDataObjectTreeColumnCollection();
		CValueDataObjectTreeColumnCollection(IValueTreeDataObject* ownerTable, IValueMetaObjectGenericData* metaObject);
		virtual ~CValueDataObjectTreeColumnCollection();

		virtual const CTypeDescription GetColumnType(unsigned int col) const {
			CDataObjectTreeColumnInfo* columnInfo = m_listColumnInfo.at(col);
			wxASSERT(columnInfo);
			return columnInfo->GetColumnType();
		}

		virtual IValueModelColumnInfo* GetColumnInfo(unsigned int idx) const {
			if (m_listColumnInfo.size() < idx)
				return nullptr;
			auto& it = m_listColumnInfo.begin();
			std::advance(it, idx);
			return it->second;
		}

		virtual unsigned int GetColumnCount() const {
			IValueMetaObjectGenericData* metaTable = m_ownerTable->GetMetaObject();
			wxASSERT(metaTable);
			const auto& obj = metaTable->GetGenericAttributeArrayObject();
			return obj.size();
		}

		//array support 
		virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
		virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	protected:

		IValueTreeDataObject* m_ownerTable;
		std::map<meta_identifier_t, CValuePtr<CDataObjectTreeColumnInfo>> m_listColumnInfo;
		CMethodHelper* m_methodHelper;
	};

	class CValueDataObjectTreeReturnLine : public IValueModelReturnLine {
		wxDECLARE_DYNAMIC_CLASS(CValueDataObjectTreeReturnLine);
	public:

		CValueDataObjectTreeReturnLine(IValueTreeDataObject* ownerTable = nullptr, const wxDataViewItem& line = wxDataViewItem(nullptr));
		virtual ~CValueDataObjectTreeReturnLine();

		virtual IValueTree* GetOwnerModel() const {
			return m_ownerTable;
		}

		virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
			//PrepareNames(); 
			return m_methodHelper;
		}

		virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

		virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal); //setting attribute
		virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal); //attribute value

	protected:
		CMethodHelper* m_methodHelper;
		IValueTreeDataObject* m_ownerTable;
	};

public:

	void AppendSort(IValueMetaObject* metaObject, bool ascending = true, bool use = true, bool system = false) {
		if (metaObject == nullptr)
			return;
		m_sortOrder.AppendSort(
			metaObject->GetMetaID(),
			metaObject->GetName(),
			metaObject->GetSynonym(),
			ascending, use, system
		);
	}

	virtual bool AutoCreateColumn() const { return false; }
	virtual bool EditableLine(const wxDataViewItem& item, unsigned int col) const { return false; }

	//set meta/get meta
	virtual bool SetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, const CValue& varMetaVal) { return false; }
	virtual bool GetValueByMetaID(const wxDataViewItem& item, const meta_identifier_t& id, CValue& pvarMetaVal) const {
		wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
		if (node == nullptr)
			return false;
		return node->GetValue(id, pvarMetaVal);
	}

	//ctor
	IValueTreeDataObject(IValueMetaObjectGenericData* metaObject = nullptr, const form_identifier_t& formType = wxNOT_FOUND, bool choiceMode = false);
	virtual ~IValueTreeDataObject();

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetSourceMetaObject() const final { return GetMetaObject(); }

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const final { return GetClassType(); }

	//Get presentation 
	virtual wxString GetSourceCaption() const {
		return GetMetaObject() ?
			stringUtils::GenerateSynonym(GetMetaObject()->GetClassName()) + wxT(": ") + GetMetaObject()->GetSynonym() : GetString();
	}

	//Is new object?
	virtual bool IsNewObject() const { return false; }

	//get unique identifier 
	virtual CUniqueKey GetGuid() const { return m_objGuid; }

	//get metaData from object 
	virtual IValueMetaObjectGenericData* GetMetaObject() const = 0;

	//counter
	virtual void SourceIncrRef() { CValue::IncrRef(); }
	virtual void SourceDecrRef() { CValue::DecrRef(); }

	virtual bool IsEmpty() const { return false; }

	//Get ref class 
	virtual class_identifier_t GetClassType() const = 0;

	virtual wxString GetClassName() const = 0;
	virtual wxString GetString() const = 0;

protected:

	CGuid m_objGuid;
	CValuePtr<CValueDataObjectTreeColumnCollection> m_recordColumnCollection;
	CMethodHelper* m_methodHelper;
};

// tree with parent or only parent 
class BACKEND_API CValueTreeDataObjectFolderRef : public IValueTreeDataObject {
	wxDECLARE_DYNAMIC_CLASS(CValueTreeDataObjectFolderRef);
public:

	enum {
		LIST_FOLDER,
		LIST_ITEM_FOLDER,
		LIST_ITEM,
	};

	struct wxValueTreeListNode : public wxValueTreeNode {
		CGuid GetGuid() const { return m_objGuid; }
		wxValueTreeListNode(wxValueTreeNode* parent, const CGuid& guid, IValueTreeDataObject* treeValue = nullptr, bool container = false) :
			wxValueTreeNode(parent), m_objGuid(guid), m_container(container) {
			m_valueTree = treeValue;
		}
		virtual bool IsContainer() const { return m_container; }
	private:
		CGuid m_objGuid;
		bool m_container;
	};

public:

	virtual wxDataViewItem FindRowValue(const CValue& varValue, const wxString& colName = wxEmptyString) const;
	virtual wxDataViewItem FindRowValue(IValueModelReturnLine* retLine) const;

	//Constructor
	CValueTreeDataObjectFolderRef(IValueMetaObjectRecordDataHierarchyMutableRef* metaObject = nullptr,
		const form_identifier_t& formType = wxNOT_FOUND, int listMode = LIST_ITEM, bool choiceMode = false);

	virtual void GetValueByRow(wxVariant& variant,
		const wxDataViewItem& item, unsigned int col) const override;
	virtual bool SetValueByRow(const wxVariant& variant,
		const wxDataViewItem& item, unsigned int col) override;
	virtual bool GetAttrByRow(const wxDataViewItem& WXUNUSED(row), unsigned int WXUNUSED(col),
		wxDataViewItemAttr& WXUNUSED(attr)) const override;

	//support source data 
	virtual CSourceExplorer GetSourceExplorer() const;
	virtual bool GetModel(IValueModel*& tableValue, const meta_identifier_t& id);

	//****************************************************************************
	//*                              Support methods                             *
	//****************************************************************************
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames(); 
		return m_methodHelper;
	}
	virtual void PrepareNames() const;
	//****************************************************************************
	//*                              Override attribute                          *
	//****************************************************************************
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	virtual bool CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray);

	//on activate item
	virtual void ActivateItem(IBackendValueForm* srcForm,
		const wxDataViewItem& item, unsigned int col) {
		if (m_choiceMode) {
			ChooseValue(srcForm);
		}
		else {
			EditValue();
		}
	}

	//get metaData from object 
	virtual IValueMetaObjectRecordDataHierarchyMutableRef* GetMetaObject() const { return m_metaObject; }

	//Get ref class 
	virtual class_identifier_t GetClassType() const;

	virtual wxString GetClassName() const;
	virtual wxString GetString() const;

	//support actionData
	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	//events:
	virtual void AddValue(unsigned int before = 0) override;
	virtual void AddFolderValue(unsigned int before = 0);
	virtual void CopyValue() override;
	virtual void EditValue() override;
	virtual void DeleteValue() override;

	virtual void MarkAsDeleteValue();
	virtual void ChooseValue(IBackendValueForm* srcForm);

private:

	//****************************************************************************
	//*                               Support model                              *
	//****************************************************************************

	virtual void RefreshModel(const wxDataViewItem& topItem = wxDataViewItem(nullptr), const int countPerPage = defaultCountPerPage);

private:

	bool m_choiceMode; int m_listMode;
	IValueMetaObjectRecordDataHierarchyMutableRef* m_metaObject;
};

#endif 