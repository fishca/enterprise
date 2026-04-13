#ifndef __TYPE_DESCRIPTION_H__
#define __TYPE_DESCRIPTION_H__

enum eDateFractions {
	eDateFractions_Date = 0,
	eDateFractions_DateTime,
	eDateFractions_Time
};

enum eAllowedLength {
	eAllowedLength_Variable,
	eAllowedLength_Fixed
};

struct CQualifierNumber {
	bool m_nonNegative;
	unsigned char m_precision;
	unsigned char m_scale;
	CQualifierNumber(unsigned char precision = 10, char scale = 0, bool nonNegative = false) :
		m_precision(precision), m_scale(scale), m_nonNegative(nonNegative) {
	}
};

struct CQualifierDate {
	eDateFractions m_dateTime;
	CQualifierDate(eDateFractions dateTime = eDateFractions::eDateFractions_DateTime) : m_dateTime(dateTime) {
	}
};

struct CQualifierString {
	unsigned short m_length;
	eAllowedLength m_allowedLength;
	CQualifierString(unsigned short length = 10, eAllowedLength allowedLength = eAllowedLength::eAllowedLength_Variable) :
		m_length(length), m_allowedLength(allowedLength) {
	}
};

#include "backend/compiler/value.h"

struct CTypeDescription {

	std::vector<class_identifier_t> m_listTypeClass;

	struct CTypeData {
		CQualifierNumber m_number;
		CQualifierDate	 m_date;
		CQualifierString m_string;
	public:

		CTypeData() :
			m_number(10, 0), m_date(eDateFractions::eDateFractions_Date), m_string(10) {
		} //empty 
		CTypeData(unsigned char precision, unsigned char scale, bool nonnegative = false) :
			m_number(precision, scale, nonnegative), m_date(eDateFractions::eDateFractions_Date), m_string(10) {
		}
		CTypeData(eDateFractions dateTime) :
			m_number(10, 0), m_date(dateTime), m_string(10) {
		}
		CTypeData(unsigned short length, eAllowedLength allowedLength = eAllowedLength::eAllowedLength_Variable) :
			m_number(10, 0), m_date(eDateFractions::eDateFractions_Date), m_string(length, allowedLength) {
		}
		CTypeData(const CQualifierNumber& qNumber, const CQualifierDate& qDate, const CQualifierString& qString) :
			m_number(qNumber), m_date(qDate), m_string(qString) {
		}

		//get special data number 
		unsigned char GetPrecision() const {
			return m_number.m_precision;
		}

		unsigned char GetScale() const {
			return m_number.m_scale;
		}

		bool IsNonNegative() const {
			return m_number.m_nonNegative;
		}

		//get special data date  
		eDateFractions GetDateFraction() const {
			return m_date.m_dateTime;
		}

		//get special data string  
		unsigned short GetLength() const {
			return m_string.m_length;
		}

		eAllowedLength GetAllowedLength() const {
			return m_string.m_allowedLength;
		}

		void SetNumber(unsigned char precision, unsigned char scale, bool nonNegative = false) {
			m_number.m_precision = precision;
			m_number.m_scale = scale;
			m_number.m_nonNegative = nonNegative;
		}

		void SetDate(eDateFractions dateTime) {
			m_date.m_dateTime = dateTime;
		}

		void SetString(unsigned short length, eAllowedLength allowedLength = eAllowedLength::eAllowedLength_Variable) {
			m_string.m_length = length;
			m_string.m_allowedLength = allowedLength;
		}
	};

	CTypeData m_typeData;

public:

	bool IsOk() const { return m_listTypeClass.size() > 0; }

	class_identifier_t GetFirstClsid() const {
		if (m_listTypeClass.size() == 0)
			return 0;
		auto it = m_listTypeClass.begin();
		std::advance(it, 0);
		return *it;
	}

	const std::vector<class_identifier_t>& GetClsidList() const { return m_listTypeClass; }

	//get special data number 
	unsigned char GetPrecision() const { return m_typeData.GetPrecision(); }
	unsigned char GetScale() const { return m_typeData.GetScale(); }

	bool IsNonNegative() const { return m_typeData.IsNonNegative(); }

	//get special data date  
	eDateFractions GetDateFraction() const { return m_typeData.GetDateFraction(); }

	//get special data string  
	unsigned short GetLength() const { return m_typeData.GetLength(); }
	eAllowedLength GetAllowedLength() const { return m_typeData.GetAllowedLength(); }

	////////////////////////////////////////////////////////////////////////////////////////////////

	void SetDefaultMetaType(const CTypeDescription& typeDesc) {
		ClearMetaType();
		AppendMetaType(typeDesc.m_listTypeClass, typeDesc.m_typeData);
	}

	void SetDefaultMetaType(const eValueTypes& valType) {
		ClearMetaType();
		AppendMetaType(valType);
	}

	void SetDefaultMetaType(const class_identifier_t& clsid) {
		ClearMetaType();
		AppendMetaType(clsid);
	}

	void SetDefaultMetaType(const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr) {
		ClearMetaType();
		AppendMetaType(clsid, descr);
	}

	void SetDefaultMetaType(const std::vector<class_identifier_t>& array) {
		ClearMetaType();
		AppendMetaType(array);
	}

	void SetDefaultMetaType(const std::vector<class_identifier_t>& array, const CTypeDescription::CTypeData& descr) {
		ClearMetaType();
		AppendMetaType(array, descr.m_number, descr.m_date, descr.m_string);
	}

	void SetDefaultMetaType(const std::vector<class_identifier_t>& array,
		const CQualifierNumber& qNumber, const CQualifierDate& qDate, CQualifierString& qString) {
		ClearMetaType();
		AppendMetaType(array, qNumber, qDate, qString);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void AppendMetaType(const eValueTypes& valType) {

		if (valType == eValueTypes::TYPE_NUMBER) m_typeData.SetNumber(10, 0);
		if (valType == eValueTypes::TYPE_DATE) m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
		if (valType == eValueTypes::TYPE_STRING) m_typeData.SetString(10);

		const class_identifier_t clsid = CValue::GetIDByVT(valType);
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
		if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
	}

	void AppendMetaType(const class_identifier_t& clsid) {

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_NUMBER)) m_typeData.SetNumber(10, 0);
		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_DATE)) m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_STRING)) m_typeData.SetString(10);

		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
		if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
	}

	void AppendMetaType(const class_identifier_t& clsid, const CTypeDescription::CTypeData& descr) {

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_NUMBER)) {
			m_typeData.SetNumber(descr.GetPrecision(), descr.GetScale(), descr.IsNonNegative());
		}

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_DATE)) {
			m_typeData.SetDate(descr.GetDateFraction());
		}

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_STRING)) {
			m_typeData.SetString(descr.GetLength(), descr.GetAllowedLength());
		}

		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
		if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
	}

	void AppendMetaType(const std::vector<class_identifier_t>& array) {

		auto iterator_number = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		if (iterator_number != array.end()) m_typeData.SetNumber(10, 0);
		auto iterator_date = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_DATE));
		if (iterator_date != array.end()) m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
		auto iterator_string = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		if (iterator_string != array.end()) m_typeData.SetString(10);

		for (auto clsid : array) {
			auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
			if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
		}
	}

	void AppendMetaType(const std::vector<class_identifier_t>& array, const CTypeDescription::CTypeData& descr) {

		auto iterator_number = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		if (iterator_number != array.end()) m_typeData.SetNumber(descr.GetPrecision(), descr.GetScale(), descr.IsNonNegative());
		auto iterator_date = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_DATE));
		if (iterator_date != array.end()) m_typeData.SetDate(descr.GetDateFraction());
		auto iterator_string = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		if (iterator_string != array.end()) m_typeData.SetString(descr.GetLength(), descr.GetAllowedLength());

		for (auto clsid : array) {
			auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
			if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
		}
	}

	void AppendMetaType(const std::vector<class_identifier_t>& array,
		const CQualifierNumber& qNumber, const CQualifierDate& qDate, const CQualifierString& qString) {

		auto iterator_number = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		if (iterator_number != array.end()) m_typeData.SetNumber(qNumber.m_precision, qNumber.m_scale);
		auto iterator_date = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_DATE));
		if (iterator_date != array.end()) m_typeData.SetDate(qDate.m_dateTime);
		auto iterator_string = std::find(array.begin(), array.end(), CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		if (iterator_string != array.end()) m_typeData.SetString(qString.m_length);

		for (auto clsid : array) {
			auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
			if (iterator == m_listTypeClass.end()) m_listTypeClass.emplace_back(clsid);
		}
	}

	void AppendMetaType(const CTypeDescription& typeDesc) {
		AppendMetaType(typeDesc.m_listTypeClass, typeDesc.m_typeData);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void ClearMetaType() {
		m_typeData.SetNumber(10, 0);
		m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
		m_typeData.SetString(10);
		m_listTypeClass.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	void ClearMetaType(const class_identifier_t& clsid) {

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_NUMBER)) {
			m_typeData.SetNumber(10, 0);
		}

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_DATE)) {
			m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
		}

		if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_STRING)) {
			m_typeData.SetString(10);
		}

		m_listTypeClass.erase(
			std::remove(m_listTypeClass.begin(), m_listTypeClass.end(), clsid),
			m_listTypeClass.end());
	}

	void ClearMetaType(const std::vector<class_identifier_t>& array) {

		for (const auto clsid : array) {

			if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_NUMBER)) {
				m_typeData.SetNumber(10, 0);
			}
			if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_DATE)) {
				m_typeData.SetDate(eDateFractions::eDateFractions_DateTime);
			}
			if (clsid == CValue::GetIDByVT(eValueTypes::TYPE_STRING)) {
				m_typeData.SetString(10);
			}

			m_listTypeClass.erase(
				std::remove(m_listTypeClass.begin(), m_listTypeClass.end(), clsid),
				m_listTypeClass.end());
		}
	}

	void ClearMetaType(const CTypeDescription& typeDesc) { ClearMetaType(typeDesc.m_listTypeClass); }

	//////////////////////////////////////////////////
	void SetTypeData(const CTypeDescription::CTypeData& typeData) { m_typeData = typeData; }
	const CTypeDescription::CTypeData& GetTypeData() const { return m_typeData; }
	//////////////////////////////////////////////////

	void SetNumber(unsigned char precision, unsigned char scale, bool nonNegative = false) {
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(),
			CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		if (iterator == m_listTypeClass.end())
			m_listTypeClass.emplace_back(CValue::GetIDByVT(eValueTypes::TYPE_NUMBER));
		m_typeData.SetNumber(precision, scale, nonNegative);
	}

	void SetDate(eDateFractions dateTime) {
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(),
			CValue::GetIDByVT(eValueTypes::TYPE_DATE));
		if (iterator == m_listTypeClass.end())
			m_listTypeClass.emplace_back(CValue::GetIDByVT(eValueTypes::TYPE_DATE));
		m_typeData.SetDate(dateTime);
	}

	void SetString(unsigned short length, eAllowedLength allowedLength = eAllowedLength::eAllowedLength_Variable) {
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(),
			CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		if (iterator == m_listTypeClass.end())
			m_listTypeClass.emplace_back(CValue::GetIDByVT(eValueTypes::TYPE_STRING));
		m_typeData.SetString(length, allowedLength);
	}

	//qualifiers 
	const CQualifierNumber& GetNumberQualifier() const { return m_typeData.m_number; }
	const CQualifierDate& GetDateQualifier() const { return m_typeData.m_date; }
	const CQualifierString& GetStringQualifier() const { return m_typeData.m_string; }

	//////////////////////////////////////////////////

	class_identifier_t GetByIdx(unsigned int idx) const {
		if (idx > m_listTypeClass.size())
			return wxNOT_FOUND;
		return m_listTypeClass[idx];
	}

	//////////////////////////////////////////////////

	unsigned int GetClsidCount() const { return m_listTypeClass.size(); }

	//////////////////////////////////////////////////

	CTypeDescription() {}
	CTypeDescription(const class_identifier_t& clsid) : m_listTypeClass({ clsid }) {}
	CTypeDescription(const class_identifier_t& clsid, const CTypeData& descr) : m_listTypeClass({ clsid }), m_typeData(descr) {}
	CTypeDescription(const class_identifier_t& clsid, const CQualifierNumber& qNumber, const CQualifierDate& qDate, const CQualifierString& qString) : m_listTypeClass({ clsid }), m_typeData(qNumber, qDate, qString) {}
	CTypeDescription(const std::vector<class_identifier_t>& array) : m_listTypeClass(array) {}
	CTypeDescription(const std::vector<class_identifier_t>& array, const CTypeData& descr) : m_listTypeClass(array), m_typeData(descr) {}
	CTypeDescription(const std::vector<class_identifier_t>& array, const CQualifierNumber& qNumber, const CQualifierDate& qDate, const CQualifierString& qString) : m_listTypeClass(array), m_typeData(qNumber, qDate, qString) {}

	bool ContainType(const eValueTypes& valType) const {
		if (valType == eValueTypes::TYPE_ENUM) {
			for (auto clsid : m_listTypeClass) {
				if (CValue::IsRegisterCtor(clsid)) {
					if (CValue::GetVTByID(clsid) == eValueTypes::TYPE_ENUM)
						return true;
				}
			}
			return false;
		}
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), CValue::GetIDByVT(valType));
		return iterator != m_listTypeClass.end();
	}

	bool ContainType(const class_identifier_t& clsid) const {
		auto iterator = std::find(m_listTypeClass.begin(), m_listTypeClass.end(), clsid);
		return iterator != m_listTypeClass.end();
	}

	bool EqualType(const class_identifier_t& clsid, const CTypeDescription& rhs) const {
		eValueTypes valType = CValue::GetVTByID(clsid);
		if (valType == eValueTypes::TYPE_NUMBER) {
			bool result = m_typeData.m_number.m_precision == rhs.m_typeData.m_number.m_precision &&
				m_typeData.m_number.m_scale == rhs.m_typeData.m_number.m_scale &&
				m_typeData.m_number.m_nonNegative == rhs.m_typeData.m_number.m_nonNegative;
			if (!result)
				return false;
		}
		else if (valType == eValueTypes::TYPE_DATE) {
			bool result = m_typeData.m_date.m_dateTime == rhs.m_typeData.m_date.m_dateTime;
			if (!result)
				return false;
		}
		else if (valType == eValueTypes::TYPE_STRING) {
			bool result = m_typeData.m_string.m_length == rhs.m_typeData.m_string.m_length &&
				m_typeData.m_string.m_allowedLength == rhs.m_typeData.m_string.m_allowedLength;
			if (!result)
				return false;
		}

		return ContainType(clsid);
	}

	bool operator == (const CTypeDescription& rhs) const {
		if (m_listTypeClass == rhs.m_listTypeClass) {
			for (auto clsid : m_listTypeClass) {
				eValueTypes valType = CValue::GetVTByID(clsid);
				if (valType == eValueTypes::TYPE_NUMBER) {
					bool result = m_typeData.m_number.m_precision == rhs.m_typeData.m_number.m_precision &&
						m_typeData.m_number.m_scale == rhs.m_typeData.m_number.m_scale &&
						m_typeData.m_number.m_nonNegative == rhs.m_typeData.m_number.m_nonNegative;
					if (!result)
						return false;
				}
				else if (valType == eValueTypes::TYPE_DATE) {
					bool result = m_typeData.m_date.m_dateTime == rhs.m_typeData.m_date.m_dateTime;
					if (!result)
						return false;
				}
				else if (valType == eValueTypes::TYPE_STRING) {
					bool result = m_typeData.m_string.m_length == rhs.m_typeData.m_string.m_length &&
						m_typeData.m_string.m_allowedLength == rhs.m_typeData.m_string.m_allowedLength;
					if (!result)
						return false;
				}
			}
			return true;
		}

		return false;
	}

	bool operator != (const CTypeDescription& rhs) const {
		if (m_listTypeClass == rhs.m_listTypeClass) {
			for (auto clsid : m_listTypeClass) {
				eValueTypes valType = CValue::GetVTByID(clsid);
				if (valType == eValueTypes::TYPE_NUMBER) {
					bool result = m_typeData.m_number.m_precision != rhs.m_typeData.m_number.m_precision ||
						m_typeData.m_number.m_scale != rhs.m_typeData.m_number.m_scale ||
						m_typeData.m_number.m_nonNegative != rhs.m_typeData.m_number.m_nonNegative;
					if (result)
						return true;

				}
				else if (valType == eValueTypes::TYPE_DATE) {
					bool result = m_typeData.m_date.m_dateTime != rhs.m_typeData.m_date.m_dateTime;
					if (result)
						return true;
				}
				else if (valType == eValueTypes::TYPE_STRING) {
					bool result = m_typeData.m_string.m_length != rhs.m_typeData.m_string.m_length ||
						m_typeData.m_string.m_allowedLength != rhs.m_typeData.m_string.m_allowedLength;
					if (result)
						return true;
				}
			}
			return false;
		}
		return true;
	}

	bool LoadMetaType(const CTypeDescription& rhs) {
		ClearMetaType();
		m_typeData = rhs.m_typeData;
		m_listTypeClass = rhs.m_listTypeClass;
		return true;
	}
};

class BACKEND_API CTypeDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CTypeDescription& typeDesc);
	static bool SaveData(class CMemoryWriter& writer, CTypeDescription& typeDesc);
};

struct CMetaDescription {
	std::vector<meta_identifier_t> m_listMetaClass;
public:
	CMetaDescription() {}
	CMetaDescription(const meta_identifier_t& id) : m_listMetaClass({ id }) {}
	CMetaDescription(const std::vector<meta_identifier_t>& array) : m_listMetaClass(array) {}
public:

	bool IsOk() const { return m_listMetaClass.size() > 0; }

	void SetDefaultMetaType(const meta_identifier_t& id) {
		ClearMetaType();
		AppendMetaType(id);
	}

	void SetDefaultMetaType(const CMetaDescription& typeDesc) {
		ClearMetaType();
		AppendMetaType(typeDesc.m_listMetaClass);
	}

	void AppendMetaType(const meta_identifier_t& id) { m_listMetaClass.emplace_back(id); }
	void AppendMetaType(const std::vector<meta_identifier_t>& array) { for (auto& id : array) m_listMetaClass.emplace_back(id); }

	void ClearMetaType() { m_listMetaClass.clear(); }
	bool ContainMetaType(const meta_identifier_t& id) const {
		auto iterator = std::find(m_listMetaClass.begin(), m_listMetaClass.end(), id);
		return iterator != m_listMetaClass.end();
	}

	meta_identifier_t GetByIdx(unsigned int idx) const {
		if (idx > m_listMetaClass.size())
			return wxNOT_FOUND;
		return m_listMetaClass[idx];
	}

	unsigned int GetTypeCount() const { return m_listMetaClass.size(); }
};

class BACKEND_API CMetaDescriptionMemory {
public:
	//load & save object in control 
	static bool LoadData(class CMemoryReader& reader, CMetaDescription& metaDesc);
	static bool SaveData(class CMemoryWriter& writer, CMetaDescription& metaDesc);
};

#endif