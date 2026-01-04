#ifndef __METADATA_H__
#define __METADATA_H__

#include "backend/moduleManager/moduleManager.h"

///////////////////////////////////////////////////////////////////////////////
class BACKEND_API IBackendMetadataTree;
///////////////////////////////////////////////////////////////////////////////
class BACKEND_API IMetaValueTypeCtor;
///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IMetaData {
	void DoGenerateNewID(meta_identifier_t& id, IMetaObject* top) const;
public:

	IMetaData() :
		m_metaTree(nullptr),
		m_metaModify(false) {
	}

	virtual ~IMetaData() {}

	virtual IModuleManager* GetModuleManager() const = 0;

	virtual bool IsModified() const { return m_metaModify; }
	virtual void Modify(bool modify = true) {
		if (m_metaTree != nullptr)
			m_metaTree->Modify(modify);
		m_metaModify = modify;
	}

	virtual void SetVersion(const version_identifier_t& version) = 0;
	virtual version_identifier_t GetVersion() const = 0;

	virtual wxString GetFileName() const { return wxEmptyString; }
	virtual IMetaObject* GetCommonMetaObject() const { return nullptr; }

	//runtime support:
	inline CValue CreateObject(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CreateObjectRef(clsid, paParams, lSizeArray);
	}
	inline CValue CreateObject(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CreateObjectRef(className, paParams, lSizeArray);
	}

	template<typename T, typename... Args>
	inline CValue CreateObjectValue(Args&&... args) const {
		return CreateObjectValueRef<T>(std::forward<Args>(args)...);
	}

	virtual CValue* CreateObjectRef(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) const;
	virtual CValue* CreateObjectRef(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		const class_identifier_t& clsid = GetIDObjectFromString(className);
		return CreateObjectRef(clsid, paParams, lSizeArray);
	}
	template<typename T, typename... Args>
	inline CValue* CreateObjectValueRef(Args&&... args) const {
		return CreateAndConvertObjectValueRef<T>(std::forward<Args>(args)...);
	}

	template<class T = CValue>
	inline T* CreateAndConvertObjectRef(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CastValue<T>(CreateObjectRef(clsid, paParams, lSizeArray));
	}
	template<class T = CValue>
	inline T* CreateAndConvertObjectRef(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) const {
		return CastValue<T>(CreateObjectRef(className, paParams, lSizeArray));
	}
	template<typename T, typename... Args>
	inline T* CreateAndConvertObjectValueRef(Args&&... args) const {
		T* created_value = CValue::CreateAndPrepareValueRef<T>(args...);
		if (!IsRegisterCtor(created_value->GetClassType())) {
			wxDELETE(created_value);
			wxASSERT_MSG(false, "CreateAndConvertObjectValueRef ret null!");
			return nullptr;
		}
		return created_value;
	}

	void RegisterCtor(IMetaValueTypeCtor* typeCtor);
	void UnRegisterCtor(IMetaValueTypeCtor*& typeCtor);

	void UnRegisterCtor(const wxString& className);

	virtual bool IsRegisterCtor(const wxString& className) const;
	virtual bool IsRegisterCtor(const wxString& className, eCtorObjectType objectType) const;
	virtual bool IsRegisterCtor(const wxString& className, eCtorObjectType objectType, enum eCtorMetaType metaType) const;

	virtual bool IsRegisterCtor(const class_identifier_t& clsid) const;

	virtual class_identifier_t GetIDObjectFromString(const wxString& className) const;
	virtual wxString GetNameObjectFromID(const class_identifier_t& clsid, bool upper = false) const;

	inline meta_identifier_t GetVTByID(const class_identifier_t& clsid) const;
	inline class_identifier_t GetIDByVT(const meta_identifier_t& valueType, enum eCtorMetaType refType) const;

	virtual IMetaValueTypeCtor* GetTypeCtor(const wxString& className) const;
	virtual IMetaValueTypeCtor* GetTypeCtor(const class_identifier_t& clsid) const;
	virtual IMetaValueTypeCtor* GetTypeCtor(const IMetaObject* metaValue, enum eCtorMetaType refType) const;

	virtual IAbstractTypeCtor* GetAvailableCtor(const wxString& className) const;
	virtual IAbstractTypeCtor* GetAvailableCtor(const class_identifier_t& clsid) const;

	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType() const;
	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType(const class_identifier_t& clsid, enum eCtorMetaType refType) const;
	virtual std::vector<IMetaValueTypeCtor*> GetListCtorsByType(enum eCtorMetaType refType) const;

	//get parent metadata 
	virtual bool GetOwner(IMetaData*& metaData) const { return false; }

	//factory version 
	virtual unsigned int GetFactoryCountChanges() const {
		return m_factoryCtorCountChanges + CValue::GetFactoryCountChanges();
	}

	//get language code 
	virtual wxString GetLangCode() const = 0;

	//Check is full access 
	virtual bool IsFullAccess() const { return true; }

	//associate this metaData with 
	virtual IBackendMetadataTree* GetMetaTree() const { return m_metaTree; }
	virtual void SetMetaTree(IBackendMetadataTree* metaTree) { m_metaTree = metaTree; }

	//run/close 
	virtual bool RunDatabase(int flags = defaultFlag) = 0;
	virtual bool CloseDatabase(int flags = defaultFlag) = 0;

	//metaobject
	IMetaObject* CreateMetaObject(const class_identifier_t& clsid,
		IMetaObject* parentMetaObj, bool runObject = true);

	bool RenameMetaObject(IMetaObject* object, const wxString& newName);
	void RemoveMetaObject(IMetaObject* object, IMetaObject* objParent = nullptr);

#pragma region __array_h__

	//any  
	template <typename _T1 = IMetaObject>
	std::vector<_T1*> GetAnyArrayObject() const {
		std::vector<_T1*> array;
		FillArrayObjectByFilter<_T1, IMetaObject>(array, {});
		return array;
	}

	//any 
	template <typename _T1 = IMetaObject>
	std::vector<_T1*> GetAnyArrayObject(const class_identifier_t& clsid) const {
		std::vector<_T1*> array;
		FillArrayObjectByFilter<_T1, IMetaObject>(array, { clsid });
		return array;
	}

	//any  
	template <typename _T1 = IMetaObject>
	std::vector<_T1*> GetAnyArrayObject(const std::initializer_list<class_identifier_t> filter) const {
		std::vector<_T1*> array;
		FillArrayObjectByFilter<_T1, IMetaObject>(array, filter);
		return array;
	}

#pragma endregion
#pragma region __filter_h__

	//any 
	template <typename _T1 = IMetaObject, typename _T2>
	_T1* FindAnyObjectByFilter(const _T2& id) const {
		return FindObjectByFilter<_T2, IMetaObject, _T1>(id, {});
	}

	//any 
	template <typename _T1 = IMetaObject, typename _T2>
	_T1* FindAnyObjectByFilter(const _T2& id, const class_identifier_t& clsid) const {
		return FindObjectByFilter<_T2, IMetaObject, _T1>(id, { clsid });
	}

	//any 
	template <typename _T1 = IMetaObject, typename _T2>
	_T1* FindAnyObjectByFilter(const _T2& id,
		const std::initializer_list<class_identifier_t> filter) const {
		return FindObjectByFilter<_T2, IMetaObject, IMetaObject, _T1>(id, filter);
	}

#pragma endregion 

	//ID's 
	meta_identifier_t GenerateNewID() const;

	//generate new name
	wxString GetNewName(const class_identifier_t& clsid,
		IMetaObject* parent, const wxString& strPrefix = wxEmptyString, bool forConstructor = false);

protected:

#pragma region __array_h__

	template <typename _T1 = IMetaObject, typename _T2 = IMetaObject>
	bool FillArrayObjectByFilter(
		std::vector<_T1*>& array,
		const std::initializer_list<class_identifier_t> filter) const
	{
		const auto commonObject = GetCommonMetaObject();
		if (commonObject != nullptr)
			return commonObject->FillArrayObjectByFilter(array, filter, false);
		return false;
	}

#pragma endregion 
#pragma region __filter_h__

	template<typename _T1, typename _T2 = IMetaObject, typename _T3 = IMetaObject>
	_T3* FindObjectByFilter(
		const _T1& id,
		const std::initializer_list<class_identifier_t> filter) const {
		const auto commonObject = GetCommonMetaObject();
		if (commonObject != nullptr)
			return commonObject->FindObjectByFilter<_T3>(id, filter, false);
		return nullptr;
	}

#pragma endregion 

	enum
	{
		eHeaderBlock = 0x2320,
		eDataBlock = 0x2350,
		eChildBlock = 0x2370
	};

	bool m_metaModify;

	//custom types
	std::vector<IMetaValueTypeCtor*> m_factoryCtors;
	std::atomic<unsigned int> m_factoryCtorCountChanges = 0;

private:
	IBackendMetadataTree* m_metaTree;
};

#endif 