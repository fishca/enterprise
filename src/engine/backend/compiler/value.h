#ifndef __VALUE_H__
#define __VALUE_H__

#include "backend/backend_core.h"
#include "backend/compiler/typeCtor.h"

extern BACKEND_API const CValue wxEmptyValue;

class BACKEND_API IBackendValue {
public:
	virtual CValue* GetImplValueRef() const = 0;
};

const class_identifier_t g_valueUndefinedCLSID = string_to_clsid("VL_UNDF");

const class_identifier_t g_valueBooleanCLSID = string_to_clsid("VL_BOOL");
const class_identifier_t g_valueNumberCLSID = string_to_clsid("VL_NUMB");
const class_identifier_t g_valueDateCLSID = string_to_clsid("VL_DATE");
const class_identifier_t g_valueStringCLSID = string_to_clsid("VL_STRI");

const class_identifier_t g_valueNullCLSID = string_to_clsid("VL_NULL");

//simple type date
class BACKEND_API CValue : public wxObject {
	wxDECLARE_DYNAMIC_CLASS(CValue);
public:
	bool m_bReadOnly;
	//ATTRIBUTES:
	eValueTypes m_typeClass;
	union {
		bool          m_bData;  //TYPE_BOOL
		number_t      m_fData;  //TYPE_NUMBER
		wxLongLong_t  m_dData;  //TYPE_DATE
		CValue* m_pRef;	//TYPE_REFFER
	};
	wxString m_sData;  //TYPE_STRING
public:

	class BACKEND_API CMethodHelper {

		//List of keywords that cannot be variable and function names
		struct CFieldConstructor {

			CFieldConstructor(const wxString& strHelper, const long paramCount, const long lPropAlias = wxNOT_FOUND, const long lData = wxNOT_FOUND)
				: m_strHelper(strHelper), m_paramCount(paramCount), m_lAlias(lPropAlias), m_lData(lData)
			{
			}

			wxString m_strHelper;
			long m_paramCount = 0;
			long m_lAlias, m_lData;
		};

		struct CFieldProperty {

			CFieldProperty(const wxString& strPropName, bool readable, bool writable, const long lPropAlias = wxNOT_FOUND, const long lData = wxNOT_FOUND)
				: m_fieldName(strPropName), m_readable(readable), m_writable(writable), m_lAlias(lPropAlias), m_lData(lData)
			{
			}

			wxString m_fieldName;
			bool m_readable = true;
			bool m_writable = true;
			long m_lAlias, m_lData;
		};

		struct CFieldMethod {

			CFieldMethod(const wxString& strMethodName, const wxString& strHelper, const long paramCount, bool hasRet, const long lPropAlias = wxNOT_FOUND, const long lData = wxNOT_FOUND)
				: m_fieldName(strMethodName), m_strHelper(strHelper), m_paramCount(paramCount), m_hasRet(hasRet), m_lAlias(lPropAlias), m_lData(lData)
			{
			}

			wxString m_fieldName;
			wxString m_strHelper;
			long m_paramCount = 0;
			bool m_hasRet = true;
			long m_lAlias, m_lData;
		};

		// constructors & props & methods
		std::vector<CFieldConstructor> m_constructorHelper; // tree of constructor names
		std::vector<CFieldProperty> m_propHelper; // tree of attribute names
		std::vector<CFieldMethod> m_methodHelper; // tree of method names

	public:

		CMethodHelper() {}

		void ClearHelper() {
			m_constructorHelper.clear();
			m_propHelper.clear();
			m_methodHelper.clear();
		}

		inline long AppendConstructor(const wxString& strHelper) { return AppendConstructor(0, strHelper, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendConstructor(const wxString& strHelper, const long lCtorNum) { return AppendConstructor(0, strHelper, wxNOT_FOUND, lCtorNum); }
		inline long AppendConstructor(const long paramCount, const wxString& strHelper, const long lCtorNum) { return AppendConstructor(paramCount, strHelper, wxNOT_FOUND, lCtorNum); }
		inline long AppendConstructor(const long paramCount, const wxString& strHelper) { return AppendConstructor(paramCount, strHelper, wxNOT_FOUND, wxNOT_FOUND); }

		inline long AppendConstructor(const long paramCount, const wxString& strHelper, const long lCtorNum, const long lCtorAlias) {

			//auto iterator = std::find_if(m_constructorHelper.begin(), m_constructorHelper.end(),
			//	[strHelper](const auto& f) { return stringUtils::CompareString(f.m_strHelper, strHelper); });
			//if (iterator != m_constructorHelper.end())
			//	return std::distance(m_constructorHelper.begin(), iterator);

			m_constructorHelper.emplace_back(strHelper, paramCount, lCtorAlias, lCtorNum);
			return m_constructorHelper.size();
		}

		void CopyConstructor(const CMethodHelper* src, const long lCtorNum) {
			if (lCtorNum < src->GetNConstructors()) {
				m_constructorHelper.push_back(src->m_constructorHelper[lCtorNum]);
			}
		}

		wxString GetConstructorHelper(const long lCtorNum) const {
			if (lCtorNum > GetNConstructors())
				return wxEmptyString;
			return m_constructorHelper[lCtorNum].m_strHelper;
		}

		long GetConstructorAlias(const long lCtorNum) const {
			if (lCtorNum > GetNConstructors())
				return wxNOT_FOUND;
			return m_constructorHelper[lCtorNum].m_lAlias;
		}

		long GetConstructorData(const long lCtorNum) const {
			if (lCtorNum > GetNConstructors())
				return wxNOT_FOUND;
			return m_constructorHelper[lCtorNum].m_lData;
		}

		const long int GetNConstructors() const noexcept { return m_constructorHelper.size(); }

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////

		inline long AppendProp(const wxString& strPropName) { return AppendProp(strPropName, true, true, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendProp(const wxString& strPropName, const long lPropNum) { return AppendProp(strPropName, true, true, lPropNum, wxNOT_FOUND); }
		inline long AppendProp(const wxString& strPropName, const long lPropNum, const long lPropAlias) { return AppendProp(strPropName, true, true, lPropNum, lPropAlias); }
		inline long AppendProp(const wxString& strPropName, bool readable, const long lPropNum, const long lPropAlias) { return AppendProp(strPropName, readable, true, lPropNum, lPropAlias); }
		inline long AppendProp(const wxString& strPropName, bool readable, bool writable, const long lPropNum) { return AppendProp(strPropName, readable, writable, lPropNum, wxNOT_FOUND); }

		inline long AppendProp(const wxString& strPropName, bool readable, bool writable, const long lPropNum, const long lPropAlias) {

			//auto iterator = std::find_if(m_propHelper.begin(), m_propHelper.end(),
			//	[strPropName](const auto& f) { return stringUtils::CompareString(f.m_fieldName, strPropName); });
			//if (iterator != m_propHelper.end())
			//	return std::distance(m_propHelper.begin(), iterator);

			m_propHelper.emplace_back(strPropName, readable, writable, lPropAlias, lPropNum);
			return m_propHelper.size();
		}

		void CopyProp(const CMethodHelper* src, const long lPropNum) {
			if (lPropNum < src->GetNProps()) {
				m_propHelper.push_back(src->m_propHelper[lPropNum]);
			}
		}

		void RemoveProp(const wxString& strPropName) {
			m_methodHelper.erase(
				std::remove_if(m_methodHelper.begin(), m_methodHelper.end(), [strPropName](const auto& f) {
					return stringUtils::CompareString(f.m_fieldName, strPropName); }), m_methodHelper.end());
		}

		long FindProp(const wxString& strPropName) const {
			auto iterator = std::find_if(m_propHelper.begin(), m_propHelper.end(),
				[strPropName](const auto& f) { return stringUtils::CompareString(f.m_fieldName, strPropName); }
			);
			if (iterator != m_propHelper.end())
				return std::distance(m_propHelper.begin(), iterator);
			return wxNOT_FOUND;
		}

		wxString GetPropName(const long lPropNum) const {
			if (lPropNum > GetNProps())
				return wxEmptyString;
			return m_propHelper[lPropNum].m_fieldName;
		}

		long GetPropAlias(const long lPropNum) const {
			if (lPropNum > GetNProps())
				return wxNOT_FOUND;
			return m_propHelper[lPropNum].m_lAlias;
		}

		long GetPropData(const long lPropNum) const {
			if (lPropNum > GetNProps())
				return wxNOT_FOUND;
			return m_propHelper[lPropNum].m_lData;
		}

		virtual bool IsPropReadable(const long lPropNum) const {
			if (lPropNum > GetNProps())
				return false;
			return m_propHelper[lPropNum].m_readable;
		}

		virtual bool IsPropWritable(const long lPropNum) const {
			if (lPropNum > GetNProps())
				return false;
			return m_propHelper[lPropNum].m_writable;
		}

		const long GetNProps() const noexcept { return m_propHelper.size(); }

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////

		inline long AppendProc(const wxString& strMethodName) { return AppendMethod(strMethodName, wxEmptyString, 0, false, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendProc(const wxString& strMethodName, const wxString& strHelper) { return AppendMethod(strMethodName, strHelper, 0, false, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendProc(const wxString& strMethodName, const wxString& strHelper, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, 0, false, lMethodNum, lMethodAlias); }
		inline long AppendProc(const wxString& strMethodName, const wxString& strHelper, const long paramCount, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, paramCount, false, lMethodNum, lMethodAlias); }
		inline long AppendProc(const wxString& strMethodName, const long paramCount, const wxString& strHelper) { return AppendMethod(strMethodName, strHelper, paramCount, false, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendProc(const wxString& strMethodName, const long paramCount, const wxString& strHelper, const long lMethodNum) { return AppendMethod(strMethodName, strHelper, paramCount, false, lMethodNum, wxNOT_FOUND); }
		inline long AppendProc(const wxString& strMethodName, const long paramCount, const wxString& strHelper, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, paramCount, false, lMethodNum, lMethodAlias); }

		inline long AppendFunc(const wxString& strMethodName) { return AppendMethod(strMethodName, wxEmptyString, 0, true, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendFunc(const wxString& strMethodName, const wxString& strHelper) { return AppendMethod(strMethodName, strHelper, 0, true, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendFunc(const wxString& strMethodName, const long paramCount, const wxString& strHelper) { return AppendMethod(strMethodName, strHelper, paramCount, true, wxNOT_FOUND, wxNOT_FOUND); }
		inline long AppendFunc(const wxString& strMethodName, const wxString& strHelper, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, 0, true, lMethodNum, lMethodAlias); }
		inline long AppendFunc(const wxString& strMethodName, const wxString& strHelper, const long paramCount, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, paramCount, true, lMethodNum, lMethodAlias); }
		inline long AppendFunc(const wxString& strMethodName, const long paramCount, const wxString& strHelper, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, paramCount, true, wxNOT_FOUND, lMethodAlias); }
		inline long AppendFunc(const wxString& strMethodName, const long paramCount, const wxString& strHelper, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, strHelper, paramCount, true, lMethodNum, lMethodAlias); }

		inline long AppendMethod(const wxString& strMethodName, const long paramCount, bool hasRet, const long lMethodNum, const long lMethodAlias) { return AppendMethod(strMethodName, wxEmptyString, paramCount, hasRet, lMethodNum, lMethodAlias); }

		inline long AppendMethod(const wxString& strMethodName, const wxString& strHelper, const long paramCount, bool hasRet, const long lMethodNum, const long lMethodAlias) {

			//auto iterator = std::find_if(m_methodHelper.begin(), m_methodHelper.end(),
			//	[strMethodName](const auto& f) { return stringUtils::CompareString(f.m_fieldName, strMethodName); });
			//if (iterator != m_methodHelper.end())
			//	return std::distance(m_methodHelper.begin(), iterator);

			m_methodHelper.emplace_back(strMethodName, strHelper, paramCount, hasRet, lMethodAlias, lMethodNum);
			return m_methodHelper.size();
		}

		void CopyMethod(const CMethodHelper* src, const long lMethodNum) {
			if (lMethodNum < src->GetNMethods()) {
				m_methodHelper.push_back(src->m_methodHelper[lMethodNum]);
			}
		}

		void RemoveMethod(const wxString& strMethodName) {
			m_methodHelper.erase(
				std::remove_if(m_methodHelper.begin(), m_methodHelper.end(), [strMethodName](const auto& f) {
					return stringUtils::CompareString(f.m_fieldName, strMethodName); }), m_methodHelper.end());
		}

		long FindMethod(const wxString& strMethodName) const {
			auto iterator = std::find_if(m_methodHelper.begin(), m_methodHelper.end(), [strMethodName](const auto& f) {
				return stringUtils::CompareString(f.m_fieldName, strMethodName); });

			if (iterator != m_methodHelper.end())
				return std::distance(m_methodHelper.begin(), iterator);
			return wxNOT_FOUND;
		}

		wxString GetMethodName(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return wxEmptyString;
			return m_methodHelper[lMethodNum].m_fieldName;
		}

		wxString GetMethodHelper(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return wxEmptyString;
			return m_methodHelper[lMethodNum].m_strHelper;
		}

		long GetMethodAlias(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return wxNOT_FOUND;
			return m_methodHelper[lMethodNum].m_lAlias;
		}

		long GetMethodData(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return wxNOT_FOUND;
			return m_methodHelper[lMethodNum].m_lData;
		}

		bool HasRetVal(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return true;
			return m_methodHelper[lMethodNum].m_hasRet;
		}

		long GetNParams(const long lMethodNum) const {
			if (lMethodNum > GetNMethods())
				return wxNOT_FOUND;
			return m_methodHelper[lMethodNum].m_paramCount;
		}

		const long GetNMethods() const noexcept { return m_methodHelper.size(); }
	};

public:

	//METHODS:
	 //constructors:
	CValue();
	CValue(const CValue& cParam);
	CValue(CValue&& cParam);
	CValue(CValue* pParam);
	CValue(IBackendValue* pParam);
	CValue(eValueTypes nType, bool readOnly = false);

	//copy constructors:
	CValue(bool cParam); //boolean
	CValue(signed int cParam); //number
	CValue(unsigned int cParam); //number
	CValue(double cParam); //number
	CValue(const number_t& cParam); //number
	CValue(wxLongLong_t cParam); //date
	CValue(const wxDateTime& cParam); //date
	CValue(int nYear, int nMonth, int nDay, unsigned short nHour = 0, unsigned short nMinute = 0, unsigned short nSecond = 0); //date

	CValue(char* sParam); //string
	CValue(wchar_t* sParam); //string
	CValue(const wxStringImpl& sParam); //string
	CValue(const wxString& sParam); //string

	//destructor:
	virtual ~CValue();

	//clear values
	inline void Reset();

	//ref counter
	void IncrRef() { wxAtomicInc(m_refCount); }
	void DecrRef() {
		wxASSERT_MSG(m_refCount > 0, "invalid ref data count");
		if (!wxAtomicDec(m_refCount)) delete this;
	}

	//operators:
	void operator = (const CValue& cParam);
	void operator = (CValue&& cParam);

	void operator = (bool cParam);
	void operator = (short cParam);
	void operator = (unsigned short cParam);
	void operator = (int cParam);
	void operator = (unsigned int cParam);
	void operator = (float cParam);
	void operator = (double cParam);
	void operator = (const number_t& cParam);
	void operator = (const wxDateTime& cParam);
	void operator = (wxLongLong_t cParam);
	void operator = (const wxString& cParam);

	void operator = (eValueTypes cParam);
	void operator = (IBackendValue* pParam);
	void operator = (CValue* pParam);

	//Implementation of comparison operators:
	bool operator > (const CValue& cParam) const { return CompareValueGT(cParam); }
	bool operator >= (const CValue& cParam) const { return CompareValueGE(cParam); }
	bool operator < (const CValue& cParam) const { return CompareValueLS(cParam); }
	bool operator <= (const CValue& cParam) const { return CompareValueLE(cParam); }
	bool operator == (const CValue& cParam) const { return CompareValueEQ(cParam); }
	bool operator != (const CValue& cParam) const { return CompareValueNE(cParam); }

	const CValue& operator+(const CValue& cParam);
	const CValue& operator-(const CValue& cParam);

	//Implementation of comparison operators:
	virtual bool CompareValueGT(const CValue& cParam) const;
	virtual bool CompareValueGE(const CValue& cParam) const;
	virtual bool CompareValueLS(const CValue& cParam) const;
	virtual bool CompareValueLE(const CValue& cParam) const;
	virtual bool CompareValueEQ(const CValue& cParam) const;
	virtual bool CompareValueNE(const CValue& cParam) const;

	//special converting
	template <typename valueType> inline valueType* ConvertToType() const {
		return CastValue<valueType>(this);
	}

	template <typename enumType> inline enumType ConvertToEnumType() const {
		class IEnumerationValue<enumType>* enumValue =
			CastValue<class IEnumerationValue<enumType>>(this);
		return enumValue ? enumValue->GetEnumValue() : enumType();
	}

	template <typename enumType> inline enumType ConvertToEnumValue() {
		class IEnumerationVariant<enumType>* enumValue =
			CastValue<class IEnumerationVariant<enumType>>(this);
		return enumValue ? enumValue->GetEnumValue() : enumType();
	}

	template <typename enumType > inline enumType ConvertToEnumValue() const {
		const class IEnumerationVariant<enumType>* enumValue =
			CastValue<class IEnumerationVariant<enumType>>(this);
		return enumValue ? enumValue->GetEnumValue() : enumType();
	}

	//convert to value
	template <typename T> inline bool ConvertToValue(T*& ptr) const {
		if (m_typeClass == eValueTypes::TYPE_REFFER) {
			CValue* non_const_value = GetRef();
			ptr = dynamic_cast<T*>(non_const_value);
			return ptr != nullptr;
		}
		else if (m_typeClass != eValueTypes::TYPE_EMPTY) {
			ptr = dynamic_cast<T*>(const_cast<CValue*>(this));
			return ptr != nullptr;
		}
		return false;
	}

public:

	//runtime support:
	template<typename T, typename... Args>
	static T* CreateAndPrepareValueRef(Args&&... args) {
		auto ptr = static_cast<T*>(malloc(sizeof(T)));
		T* created_value = ::new (ptr) T(std::forward<Args>(args)...);
		if (created_value == nullptr)
			return nullptr;
		created_value->PrepareNames();
		return created_value;
	}

	template<typename T>
	static CValue CreateObject(CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CreateObjectRef<T>(paParams, lSizeArray);
	}
	static CValue CreateObject(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CreateObjectRef(clsid, paParams, lSizeArray);
	}
	static CValue CreateObject(const wxClassInfo* classInfo, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CreateObjectRef(classInfo, paParams, lSizeArray);
	}
	static CValue CreateObject(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CreateObjectRef(className, paParams, lSizeArray);
	}
	template<typename T, typename... Args>
	static CValue CreateObjectValue(Args&&... args) {
		return CreateObjectValueRef<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	static CValue* CreateObjectRef(CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CreateObjectRef(CLASSINFO(T), paParams, lSizeArray);
	}
	static CValue* CreateObjectRef(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0);
	static CValue* CreateObjectRef(const wxClassInfo* classInfo, CValue** paParams = nullptr, const long lSizeArray = 0) {
		const class_identifier_t& clsid = GetTypeIDByRef(classInfo);
		return CreateObjectRef(clsid, paParams, lSizeArray);
	}
	static CValue* CreateObjectRef(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) {
		const class_identifier_t& clsid = GetIDObjectFromString(className);
		return CreateObjectRef(clsid, paParams, lSizeArray);
	}
	template<typename T, typename... Args>
	static CValue* CreateObjectValueRef(Args&&... args) {
		return CreateAndConvertObjectValueRef<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	static T* CreateAndConvertObjectRef(CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CastValue<T>(CreateObjectRef(CLASSINFO(T), paParams, lSizeArray));
	}
	template<class T = CValue>
	static T* CreateAndConvertObjectRef(const class_identifier_t& clsid, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CastValue<T>(CreateObjectRef(clsid, paParams, lSizeArray));
	}
	template<class T = CValue>
	static T* CreateAndConvertObjectRef(const wxClassInfo* classInfo, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CastValue<T>(CreateObjectRef(classInfo, paParams, lSizeArray));
	}
	template<class T = CValue>
	static T* CreateAndConvertObjectRef(const wxString& className, CValue** paParams = nullptr, const long lSizeArray = 0) {
		return CastValue<T>(CreateObjectRef(className, paParams, lSizeArray));
	}
	template<typename T, typename... Args>
	static T* CreateAndConvertObjectValueRef(Args&&... args) {
		const class_identifier_t& clsid = CValue::GetTypeIDByRef(CLASSINFO(T));
		if (CValue::IsRegisterCtor(clsid))
			return CreateAndPrepareValueRef<T>(args...);
		wxASSERT_MSG(false, "CreateAndConvertObjectValueRef ret null!");
		return nullptr;
	}

	template<typename T, typename valT>
	static CValue CreateEnumObject(const valT& v) {
		return CreateEnumObjectRef<T>(v);
	}

	template<typename T, typename valT>
	static CValue* CreateEnumObjectRef(const valT& v) {
		return CreateAndConvertEnumObjectRef<T>(v);
	}

	template<typename T, typename valT>
	static CValue* CreateAndConvertEnumObjectRef(const valT& v) {
		CValuePtr<IEnumeration<valT>> createdEnum(CValue::CreateAndConvertObjectRef<T>());
		wxASSERT(createdEnum != nullptr);
		return createdEnum->CreateEnumVariantValue(v);
	}

	static void RegisterCtor(IAbstractTypeCtor* typeCtor);
	static void UnRegisterCtor(IAbstractTypeCtor*& typeCtor);
	static void UnRegisterCtor(const wxString& className);

	static bool IsRegisterCtor(const wxString& className);
	static bool IsRegisterCtor(const wxString& className, eCtorObjectType objectType);
	static bool IsRegisterCtor(const class_identifier_t& clsid);

	static class_identifier_t GetTypeIDByRef(const wxClassInfo* classInfo);
	static class_identifier_t GetTypeIDByRef(const CValue* objectRef);

	static class_identifier_t GetIDObjectFromString(const wxString& className);
	static bool CompareObjectName(const wxString& className, eValueTypes valueType) {
		return stringUtils::CompareString(className, GetNameObjectFromVT(valueType));
	}
	
	static wxString GetNameObjectFromID(const class_identifier_t& clsid, bool upper = false);
	static wxString GetNameObjectFromVT(eValueTypes valueType, bool upper = false);
	static eValueTypes GetVTByID(const class_identifier_t& clsid);
	static class_identifier_t GetIDByVT(const eValueTypes& valueType);

	static IAbstractTypeCtor* GetAvailableCtor(const wxString& className);
	static IAbstractTypeCtor* GetAvailableCtor(const class_identifier_t& clsid);
	static IAbstractTypeCtor* GetAvailableCtor(const wxClassInfo* classInfo);

	static std::vector<IAbstractTypeCtor*> GetListCtorsByType(eCtorObjectType objectType = eCtorObjectType::eCtorObjectType_object_value);

	//static event 
	static void OnRegisterObject(const wxString& className, IAbstractTypeCtor* typeCtor) {}
	static void OnUnRegisterObject(const wxString& className) {}

	//factory version 
	static unsigned int GetFactoryCountChanges();

public:

	//special copy & move function
	inline void Copy(const CValue& cOld);
	inline void Move(CValue&& cOld);

	void FromDate(int& nYear, int& nMonth, int& nDay) const;
	void FromDate(int& nYear, int& nMonth, int& nDay, unsigned short& nHour, unsigned short& nMinute, unsigned short& nSecond) const;
	void FromDate(int& nYear, int& nMonth, int& nDay, int& DayOfWeek, int& DayOfYear, int& WeekOfYear) const;

	//Virtual methods:
	virtual void SetType(eValueTypes type);
	virtual eValueTypes GetType() const;

	virtual bool IsEmpty() const;

	virtual wxString GetClassName() const;
	virtual class_identifier_t GetClassType() const;

	virtual bool Init() {
		if (m_pRef != nullptr && m_typeClass == eValueTypes::TYPE_REFFER)
			return m_pRef->Init();
		return true;
	}

	virtual bool Init(CValue** paParams, const long lSizeArray) {
		if (m_pRef != nullptr && m_typeClass == eValueTypes::TYPE_REFFER)
			return m_pRef->Init(paParams, lSizeArray);
		return true;
	}

	virtual void SetValue(const CValue& varValue);

	virtual bool SetBoolean(const wxString& strValue);
	virtual bool SetNumber(const wxString& strValue);
	virtual bool SetDate(const wxString& strValue);
	virtual bool SetString(const wxString& strValue);

	virtual bool FindValue(const wxString& findData, std::vector<CValue>& listValue) const;

	void SetData(const CValue& varValue); //setting the value without changing the type

	virtual CValue GetValue(bool getThis = false);

	virtual bool GetBoolean() const;
	virtual int GetInteger() const { return GetNumber().ToInt(); }
	virtual unsigned int GetUInteger() const { return GetNumber().ToUInt(); }
	virtual double GetDouble() const { return GetNumber().ToDouble(); }
	virtual wxDateTime GetDateTime() const { return wxLongLong(GetDate()); }

	virtual number_t GetNumber() const;
	virtual wxString GetString() const;
	virtual wxLongLong_t GetDate() const;

	/////////////////////////////////////////////////////////////////////////

	virtual CValue* GetRef() const;

	/////////////////////////////////////////////////////////////////////////

	virtual void ShowValue();

	/////////////////////////////////////////////////////////////////////////

#pragma region attribute_support

	virtual CMethodHelper* GetPMethods() const { return nullptr; }

	//collect 
	virtual void PrepareNames() const;

	/// Returns number of component properties
	/**
	*  @return number of properties
	*/
	virtual long GetNProps() const;

	/// Finds property by name
	/**
	 *  @param wsPropName - property name
	 *  @return property index or -1, if iterator is not found
	 */
	virtual long FindProp(const wxString& strPropName) const;

	/// Returns property name
	/**
	 *  @param lPropNum - property index (starting with 0)
	 *  @return proeprty name or 0 if iterator is not found
	 */
	virtual wxString GetPropName(const long lPropNum) const;

	/// Returns property value
	/**
	 *  @param lPropNum - property index (starting with 0)
	 *  @return the result of
	 */
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);

	/// Sets the property value
	/**
	 *  @param lPropNum - property index (starting with 0)
	 *  @param varPropVal - the pointer to a variable for property value
	 *  @return the result of
	 */
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);

	/// Is property readable?
	/**
	 *  @param lPropNum - property index (starting with 0)
	 *  @return true if property is readable
	 */
	virtual bool IsPropReadable(const long lPropNum) const;

	/// Is property writable?
	/**
	 *  @param lPropNum - property index (starting with 0)
	 *  @return true if property is writable
	 */
	virtual bool IsPropWritable(const long lPropNum) const;

	/// Returns number of component methods
	/**
	 *  @return number of component  methods
	 */
	virtual long GetNMethods() const;

	/// Finds a method by name 
	/**
	 *  @param wsMethodName - method name
	 *  @return - method index
	 */
	virtual long FindMethod(const wxString& strMethodName) const;

	/// Returns method name
	/**
	 *  @param lMethodNum - method index(starting with 0)
	 *  @return method name or 0 if method is not found
	 */
	virtual wxString GetMethodName(const long lMethodNum) const;

	/// Returns method helper
	/**
	*  @param lMethodNum - method index(starting with 0)
	*  @return method name or 0 if method is not found
	*/
	virtual wxString GetMethodHelper(const long lMethodNum) const;

	/// Returns number of method parameters
	/**
	 *  @param lMethodNum - method index (starting with 0)
	 *  @return number of parameters
	 */
	virtual long GetNParams(const long lMethodNum) const;

	/// Returns default value of method parameter
	/**
	 *  @param lMethodNum - method index(starting with 0)
	 *  @param lParamNum - parameter index (starting with 0)
	 *  @param pvarParamDefValue - the pointer to a variable for default value
	 *  @return the result of
	 */
	virtual bool GetParamDefValue(const long lMethodNum,
		const long lParamNum,
		CValue& pvarParamDefValue) const;

	/// Does the method have a return value?
	/**
	 *  @param lMethodNum - method index (starting with 0)
	 *  @return true if the method has a return value
	 */
	virtual bool HasRetVal(const long lMethodNum) const;

	/// Calls the method as a procedure
	/**
	 *  @param lMethodNum - method index (starting with 0)
	 *  @param paParams - the pointer to array of method parameters
	 *  @param lSizeArray - the size of array
	 *  @return the result of
	 */
	virtual bool CallAsProc(const long lMethodNum,
		CValue** paParams,
		const long lSizeArray);

	/// Calls the method as a function
	/**
	 *  @param lMethodNum - method index (starting with 0)
	 *  @param pvarRetValue - the pointer to returned value
	 *  @param paParams - the pointer to array of method parameters
	 *  @param lSizeArray - the size of array
	 *  @return the result of
	 */
	virtual bool CallAsFunc(const long lMethodNum,
		CValue& pvarRetValue,
		CValue** paParams,
		const long lSizeArray);

public:

#pragma endregion

	CValue* GetThis() { return this; }

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	virtual bool SetAt(const CValue& varKeyValue, const CValue& varValue);
	virtual bool GetAt(const CValue& varKeyValue, CValue& pvarValue);

	CValue operator[](const CValue& varKeyValue) {
		CValue retValue;
		GetAt(varKeyValue, retValue);
		return retValue;
	}
#pragma region iterator_support 
	virtual bool HasIterator() const;

	virtual CValue GetIteratorEmpty();
	virtual CValue GetIteratorAt(unsigned int idx);
	virtual unsigned int GetIteratorCount() const;
#pragma endregion

private:

	unsigned int m_refCount;
};
#include "backend/value_ptr.h"
#include "backend/value_cast.h"
#endif 